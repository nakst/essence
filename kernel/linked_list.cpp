typedef struct LinkedItem {
	struct LinkedItem *previousItem;
	void *thisItem;
	struct LinkedItem *nextItem;
	struct LinkedList *list;
} LinkedItem;

typedef struct LinkedList {
	void InsertStart(LinkedItem *item);
	void InsertEnd(LinkedItem *item);
	void Remove(LinkedItem *item);

	void Validate(int from); // TODO Don't do this on release builds.

	LinkedItem *firstItem;
	LinkedItem *lastItem;

	size_t count;

	bool modCheck;
} LinkedList;

void LinkedList::InsertStart(LinkedItem *item) {
	if (modCheck) KernelPanic("LinkedList::InsertStart - Concurrent modification\n");
	modCheck = true; Defer({modCheck = false;});

	if (item->list == this) KernelPanic("LinkedList::InsertStart - Inserting an item that is already in this list\n");
	if (item->list) KernelPanic("LinkedList::InsertStart - Inserting an item that is already in a list\n");

	if (firstItem) {
		item->nextItem = firstItem;
		item->previousItem = nullptr;
		firstItem->previousItem = item;
		firstItem = item;
	} else {
		firstItem = lastItem = item;
		item->previousItem = item->nextItem = nullptr;
	}

	count++;
	item->list = this;
	Validate(0);
}

void LinkedList::InsertEnd(LinkedItem *item) {
	if (modCheck) KernelPanic("LinkedList::InsertEnd - Concurrent modification\n");
	modCheck = true; Defer({modCheck = false;});

	if (item->list == this) KernelPanic("LinkedList::InsertStart - Inserting a item that is already in this list\n");
	if (item->list) KernelPanic("LinkedList::InsertStart - Inserting a item that is already in a list\n");

	if (lastItem) {
		item->previousItem = lastItem;
		item->nextItem = nullptr;
		lastItem->nextItem = item;
		lastItem = item;
	} else {
		firstItem = lastItem = item;
		item->previousItem = item->nextItem = nullptr;
	}

	count++;
	item->list = this;
	Validate(1);
}

void LinkedList::Remove(LinkedItem *item) {
	if (modCheck) KernelPanic("LinkedList::Remove - Concurrent modification\n");
	modCheck = true; Defer({modCheck = false;});

	if (!item->list) KernelPanic("LinkedList::Remove - Removing an item that has already been removed\n");
	if (item->list != this) KernelPanic("LinkedList::Remove - Removing an item from a different list (list = %x, this = %x)\n", item->list, this);

	if (item->previousItem) {
		item->previousItem->nextItem = item->nextItem;
	} else {
		firstItem = item->nextItem;
	}

	if (item->nextItem) {
		item->nextItem->previousItem = item->previousItem;
	} else {
		lastItem = item->previousItem;
	}

	item->previousItem = item->nextItem = nullptr;
	item->list = nullptr;
	count--;
	Validate(2);
}

void LinkedList::Validate(int from) {
#ifdef DEBUG_BUILD
	if (count == 0) {
		if (firstItem || lastItem) {
			KernelPanic("LinkedList::Validate (%d) - Invalid list (1)\n", from);
		}
	} else if (count == 1) {
		if (firstItem != lastItem
				|| firstItem->previousItem
				|| firstItem->nextItem
				|| firstItem->list != this
				|| !firstItem->thisItem) {
			KernelPanic("LinkedList::Validate (%d) - Invalid list (2)\n", from);
		}
	} else {
		if (firstItem == lastItem
				|| firstItem->previousItem
				|| lastItem->nextItem) {
			KernelPanic("LinkedList::Validate (%d) - Invalid list (3) %x %x %x %x\n", from, firstItem, lastItem, firstItem->previousItem, lastItem->nextItem);
		}

		{
			LinkedItem *item = firstItem;
			size_t index = count;

			while (--index) {
				if (item->nextItem == item || item->list != this || !item->thisItem) {
					KernelPanic("LinkedList::Validate (%d) - Invalid list (4)\n", from);
				}

				item = item->nextItem;
			}

			if (item != lastItem) {
				KernelPanic("LinkedList::Validate (%d) - Invalid list (5)\n", from);
			}
		}

		{
			LinkedItem *item = lastItem;
			size_t index = count;

			while (--index) {
				if (item->previousItem == item) {
					KernelPanic("LinkedList::Validate (%d) - Invalid list (6)\n", from);
				}

				item = item->previousItem;
			}

			if (item != firstItem) {
				KernelPanic("LinkedList::Validate (%d) - Invalid list (7)\n", from);
			}
		}
	}
#else
	(void) from;
#endif
}
