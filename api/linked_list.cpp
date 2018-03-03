#ifndef KERNEL
#define LLPanic(...) OSCrashProcess(OS_FATAL_ERROR_CORRUPT_LINKED_LIST)
#else
#define LLPanic(...) KernelPanic(__VA_ARGS__)
#endif

template <class T>
struct LinkedList;

template <class T>
struct LinkedItem {
	void RemoveFromList();

	struct LinkedItem<T> *previousItem;
	T *thisItem;
	struct LinkedItem<T> *nextItem;
	struct LinkedList<T> *list;
};

template <class T>
struct LinkedList {
	void InsertStart(LinkedItem<T> *item);
	void InsertEnd(LinkedItem<T> *item);
	void Remove(LinkedItem<T> *item);

	void Validate(int from); // TODO Don't do this on release builds.

	LinkedItem<T> *firstItem;
	LinkedItem<T> *lastItem;

	size_t count;

	bool modCheck;
};

template <class T>
void LinkedItem<T>::RemoveFromList() {
	if (!list) {
		LLPanic("LinkedItem::RemoveFromList - Item not in list.\n");
	}

	list->Remove(this);
}

template <class T>
void LinkedList<T>::InsertStart(LinkedItem<T> *item) {
	if (modCheck) LLPanic("LinkedList::InsertStart - Concurrent modification\n");
	modCheck = true; Defer({modCheck = false;});

	if (item->list == this) LLPanic("LinkedList::InsertStart - Inserting an item that is already in this list\n");
	if (item->list) LLPanic("LinkedList::InsertStart - Inserting an item that is already in a list\n");

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

template <class T>
void LinkedList<T>::InsertEnd(LinkedItem<T> *item) {
	if (modCheck) LLPanic("LinkedList::InsertEnd - Concurrent modification\n");
	modCheck = true; Defer({modCheck = false;});

	if (item->list == this) LLPanic("LinkedList::InsertStart - Inserting a item that is already in this list\n");
	if (item->list) LLPanic("LinkedList::InsertStart - Inserting a item that is already in a list\n");

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

template <class T>
void LinkedList<T>::Remove(LinkedItem<T> *item) {
	if (modCheck) LLPanic("LinkedList::Remove - Concurrent modification\n");
	modCheck = true; Defer({modCheck = false;});

	if (!item->list) LLPanic("LinkedList::Remove - Removing an item that has already been removed\n");
	if (item->list != this) LLPanic("LinkedList::Remove - Removing an item from a different list (list = %x, this = %x)\n", item->list, this);

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

template <class T>
void LinkedList<T>::Validate(int from) {
#ifdef DEBUG_BUILD
	if (count == 0) {
		if (firstItem || lastItem) {
			LLPanic("LinkedList::Validate (%d) - Invalid list (1)\n", from);
		}
	} else if (count == 1) {
		if (firstItem != lastItem
				|| firstItem->previousItem
				|| firstItem->nextItem
				|| firstItem->list != this
				|| !firstItem->thisItem) {
			LLPanic("LinkedList::Validate (%d) - Invalid list (2)\n", from);
		}
	} else {
		if (firstItem == lastItem
				|| firstItem->previousItem
				|| lastItem->nextItem) {
			LLPanic("LinkedList::Validate (%d) - Invalid list (3) %x %x %x %x\n", from, firstItem, lastItem, firstItem->previousItem, lastItem->nextItem);
		}

		{
			LinkedItem<T> *item = firstItem;
			size_t index = count;

			while (--index) {
				if (item->nextItem == item || item->list != this || !item->thisItem) {
					LLPanic("LinkedList::Validate (%d) - Invalid list (4)\n", from);
				}

				item = item->nextItem;
			}

			if (item != lastItem) {
				LLPanic("LinkedList::Validate (%d) - Invalid list (5)\n", from);
			}
		}

		{
			LinkedItem<T> *item = lastItem;
			size_t index = count;

			while (--index) {
				if (item->previousItem == item) {
					LLPanic("LinkedList::Validate (%d) - Invalid list (6)\n", from);
				}

				item = item->previousItem;
			}

			if (item != firstItem) {
				LLPanic("LinkedList::Validate (%d) - Invalid list (7)\n", from);
			}
		}
	}
#else
	(void) from;
#endif
}
