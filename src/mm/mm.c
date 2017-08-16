#include <mm.h>
#include <arch.h>
#include <string.h>
#include <kernel.h>
#include <process.h>

page_t *pages;

void init_mm(u64 size)
{
	int i;
	size >>= PAGE_SHIFT;
	pages = kmalloc(size * sizeof(page_t));
	for (i = 0; i < size; i++)
		pages[i].count = 0;
}

int clone_mm(task_t *p)
{
	mm_t *mm = &p->mm;
	
	/* Copy page tables */
	pte_t *old_pg = current->mm.page_dir, *new_pg = alloc_page(MAP_READWRITE);
	if (!new_pg)
		goto out;
	memcpy(new_pg, old_pg, PAGE_SIZE);
	mm->page_dir = new_pg;
	for (int i=0; i<((MEM_BOTTOM_HALF >> 39) & 0x1ff); i++) {
		if (!(old_pg[i].flags & PTE_PRESENT))
			continue;
		pte_t *old_l3 = (pte_t *)P2V(old_pg[i].frame<<PAGE_SHIFT);
		pte_t *new_l3 = alloc_page(MAP_READWRITE);
		BUG_ON(!new_l3);
		new_pg[i].frame = V2P(new_l3) >> PAGE_SHIFT;
		memcpy(new_l3, old_l3, PAGE_SIZE);
		for (int j=0; j < PAGE_SIZE/sizeof(pte_t); j++) {
			if (!(new_l3[j].flags & PTE_PRESENT))
				continue;
			pte_t *old_l2 = (pte_t *)P2V(new_l3[j].frame<<PAGE_SHIFT);
			pte_t *new_l2 = alloc_page(MAP_READWRITE);
			BUG_ON(!new_l2);
			new_l3[j].frame = V2P(new_l2) >> PAGE_SHIFT;
			memcpy(new_l2, old_l2, PAGE_SIZE);
			for (int k=0; k < PAGE_SIZE/sizeof(pte_t); k++) {
				if (!(new_l2[k].flags & PTE_PRESENT))
					continue;
				pte_t *old_l1 = (pte_t *)P2V(new_l2[k].frame<<PAGE_SHIFT);
				pte_t *new_l1 = alloc_page(MAP_READWRITE);
				BUG_ON(!new_l1);
				new_l2[k].frame = V2P(new_l1) >> PAGE_SHIFT;
				memcpy(new_l1, old_l1, PAGE_SIZE);
				for (int l=0; l < PAGE_SIZE/sizeof(pte_t); l++) {
					if (!(new_l1[l].flags & PTE_PRESENT))
						continue;
					new_l1[l].flags &= ~PTE_WRITE;	/* for COW */
					old_l1[l].flags &= ~PTE_WRITE;	/* for COW */
					page_t *page = &pages[new_l1[l].frame];
					BUG_ON(page->count <= 0);
					page->count++;
					tlb_invalidate_entry(((u64)i<<39) + ((u64)j<<30) + ((u64)k<<21) + ((u64)l<<12));
				}
			}
		}
	}

	/* Copy vm_areas */
	list_t *vm_area_iter;
	list_init(&mm->vm_areas);
	list_for_each(&current->mm.vm_areas, vm_area_iter) {
		vm_area_t *vm_area = list_get(vm_area_iter, vm_area_t, list);
		vm_area_t *new_area = kzalloc(sizeof(vm_area_t));
		BUG_ON(!new_area);
		memcpy(new_area, vm_area, sizeof(vm_area_t));
		list_add_tail(&mm->vm_areas, &new_area->list);
	}
	return 0;
out:
	return -ENOMEM;
}

void free_mm(void)
{
	task_t *p = current;
	pte_t *l4 = p->mm.page_dir;
	for (int i=0; i<((MEM_BOTTOM_HALF >> 39) & 0x1ff); i++) {
		if (!(l4[i].flags & PTE_PRESENT))
			continue;
		pte_t *l3 = (pte_t *)P2V(l4[i].frame<<PAGE_SHIFT);
		for (int j=0; j < PAGE_SIZE/sizeof(pte_t); j++) {
			if (!(l3[j].flags & PTE_PRESENT))
				continue;
			pte_t *l2 = (pte_t *)P2V(l3[j].frame<<PAGE_SHIFT);
			for (int k=0; k < PAGE_SIZE/sizeof(pte_t); k++) {
				if (!(l2[k].flags & PTE_PRESENT))
					continue;
				pte_t *l1 = (pte_t *)P2V(l2[k].frame<<PAGE_SHIFT);
				for (int l=0; l < PAGE_SIZE/sizeof(pte_t); l++) {
					if (!(l1[l].flags & PTE_PRESENT))
						continue;
					page_t *page = &pages[l1[l].frame];
					BUG_ON(page->count <= 0);
					if (page->count == 1)
						free_page(P2V(l1[l].frame << PAGE_SHIFT));
					else
						page->count--;
				}
				free_page(l1);
			}
			free_page(l2);
		}
		free_page(l3);
	}

	list_t *vm_area_iter;
	list_for_each(&current->mm.vm_areas, vm_area_iter) {
		vm_area_t *vm_area = list_get(vm_area_iter, vm_area_t, list);
		kfree(vm_area);
		list_del(vm_area_iter);
	}
}

int mm_add_area(u64 start, u64 size, int prot, file_t *file, u64 offset)
{
	list_t *vm_area_iter;
	list_for_each(&current->mm.vm_areas, vm_area_iter) {
		vm_area_t *vm_area = list_get(vm_area_iter, vm_area_t, list);
		BUG_ON(start == vm_area->start);
		if (start < vm_area->start)
			break;
	}
	vm_area_t *new_area = kzalloc(sizeof(vm_area_t));
	if (!new_area)
		return -ENOMEM;
	new_area->start = start;
	new_area->end = start + size;
	new_area->prot = prot;
	new_area->file = file;
	new_area->offset = offset;
	list_add_before(vm_area_iter, &new_area->list);
	return 0;
}

static void mm_print_areas(void)
{
	list_t *vm_area_iter;
	list_for_each(&current->mm.vm_areas, vm_area_iter) {
		vm_area_t *vm_area = list_get(vm_area_iter, vm_area_t, list);
		printk("%p-%p %d [%s:%d]\n", vm_area->start, vm_area->end, vm_area->prot, (vm_area->file?vm_area->file->dentry->name:""), vm_area->offset);
	}
}

static vm_area_t *mm_find_area(u64 address)
{
	list_t *vm_area_iter;
	list_for_each(&current->mm.vm_areas, vm_area_iter) {
		vm_area_t *vm_area = list_get(vm_area_iter, vm_area_t, list);
		if (address >= vm_area->start && address < ROUND_PAGE_UP(vm_area->end))
			return vm_area;
	}
	return NULL;
}

static pte_t *find_user_pte(u64 addr)
{
	/* addr is not necessarily page aligned */
	addr >>= PAGE_SHIFT;
	pte_t *table = current->mm.page_dir;
	for (int i = 3; i >= 1; i--) {
		pte_t *pte = table + ((addr >> i*9) & 0x1ff);	
		if (pte->flags & PTE_PRESENT) {
			table = P2V(pte->frame << PAGE_SHIFT);
			continue;
		}
		u64 new_table = alloc_phys_page();
		BUG_ON(new_table == 0);
		table = P2V(new_table << PAGE_SHIFT);
		memset(table, 0, PAGE_SIZE);
		memset(pte, 0, sizeof(pte_t));
		pte->frame = new_table;
		pte->flags = PTE_PRESENT | PTE_WRITE | PTE_USER;
	}
	pte_t *pte = table + (addr & 0x1ff);
	return pte;
}

void *do_mmap(file_t *f, u64 offset, u64 vaddr, u64 len, int prot, int flags)
{
	if ((flags & MAP_TYPE) != MAP_PRIVATE)
		return ERR_PTR(-EINVAL);
	if (!(flags & MAP_FIXED))
		return ERR_PTR(-EINVAL);
	int ret = mm_add_area(vaddr, len, prot, f, offset);
	if (ret < 0)
		return ERR_PTR(ret);
	return (void *)vaddr;

}

void do_page_fault(regs_t *r)
{
	int present = r->error_code & 0x01;
	int write = r->error_code & 0x02;
	int usermode = r->error_code & 0x04;
	int rsvd = r->error_code & 0x08;
	int exec = r->error_code & 0x10;
	u64 address = get_cr2();

	if (rsvd) {
		goto bad;
	}
	if (present && write && usermode) {
		/* check if it's COW */
		vm_area_t *area = mm_find_area(address);
		if (!area || !(area->prot & PROT_WRITE))
			goto bad;
		pte_t *pte = find_user_pte(address);
		page_t *page = &pages[pte->frame];
		BUG_ON(page->count < 1);
		if (page->count == 1) {
			/* Just enable write */
			pte->flags |= PTE_WRITE;
		} else {
			void *oldpage = (void *)P2V(pte->frame << PAGE_SHIFT);
			void *newpage = alloc_page(0 /* flags are ignored */);
			memcpy(newpage, oldpage, PAGE_SIZE);
			pte->frame = V2P((u64)newpage) >> PAGE_SHIFT;
			pte->flags |= PTE_WRITE;
			page->count--;
		}
		tlb_invalidate_entry(address);
		return;
	}
	else if (!present && usermode) {
		vm_area_t *area = mm_find_area(address);
		if (!area) {
			goto bad;
		}
		u32 flags = PTE_PRESENT | PTE_USER;
		if ((!write && (area->prot & (PROT_READ|PROT_EXEC))) || (write && (area->prot & PROT_WRITE))) {
			if (area->prot & PROT_WRITE)
				flags |= PTE_WRITE;
		} else
			goto bad;
		pte_t *pte = find_user_pte(address);
		u64 frame = alloc_phys_page();
		if (frame == 0) {
			printk("do_page_fault: No free memory\n");
			goto bad;
		}
		void *p = (void *)P2V(frame << PAGE_SHIFT);
		if (area->file) {
			u64 offset = area->offset + (ROUND_PAGE_DOWN(address) - area->start);
			int ret = vfs_lseek(area->file, offset, SEEK_SET);
			if (ret < 0) {
				printk("do_page_fault: Can't seek in file %s to offset %ld: %d\n", area->file->dentry->name, offset, ret);
				goto bad;
			}
			u32 len = PAGE_SIZE;
			if (len > area->end - ROUND_PAGE_DOWN(address))
				len = area->end - ROUND_PAGE_DOWN(address);
			ret = vfs_read(area->file, p, len);
			if (ret < 0) {
				printk("do_page_fault: Can't read from file %s at offset %ld: %d\n", area->file->dentry->name, offset, ret);
				goto bad;
			} else if (ret != len) {
				printk("do_page_fault: Short read from file %s at offset %ld (len=%d): %d\n", area->file->dentry->name, offset, len, ret);
				goto bad;
			}
			if (len < PAGE_SIZE)
				memset((char *)p + len, 0, PAGE_SIZE - len);
		} else {
			memset(p, 0, PAGE_SIZE);
		}
		pte->frame = frame;
		pte->flags = flags;
		tlb_invalidate_entry(address);
		return;
	}
bad:
	printk("Page fault\n");
	printk("Trying to %s address (from %s mode, present=%d)%s: 0x%p\n", (exec?"execute":write?"write to":"read from"),
		(usermode?"user":"kernel"), present, (rsvd?" reserved bit set":""), address);
	oops(r);
}

