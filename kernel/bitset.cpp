#ifndef IMPLEMENTATION

struct Bitset {
	void Initialise(size_t count, bool mapAll = false);
	uintptr_t Get(size_t count = 1);
	void Put(uintptr_t index, bool intoBitset = false);

#define BITSET_STACK_SIZE (1024)
	uintptr_t stack[BITSET_STACK_SIZE];
	uintptr_t stackIndex;

#define BITSET_GROUP_SIZE (4096)
	uint32_t *singleUsage;
	uint16_t *groupUsage;

	size_t singleCount; 
	size_t groupCount;

	bool modCheck;
};

#else

void Bitset::Initialise(size_t count, bool mapAll) {
	singleCount = (count + 15) & ~15;
	groupCount = singleCount / BITSET_GROUP_SIZE + 1;

	singleUsage = (uint32_t *) kernelVMM.Allocate("Bitset", (singleCount >> 3) + (groupCount * 2), mapAll ? VMM_MAP_ALL : VMM_MAP_LAZY);
	groupUsage = (uint16_t *) singleUsage + (singleCount >> 4);
}

uintptr_t Bitset::Get(size_t count) {
	if (modCheck) KernelPanic("Bitset::Allocate - Concurrent modification.\n");
	modCheck = true; Defer({modCheck = false;});

	uintptr_t returnValue = (uintptr_t) -1;

	if (count == 1) {
		if (stackIndex) {
			returnFromStack:
			stackIndex--;
			returnValue = stack[stackIndex];
		} else {
			for (uintptr_t i = 0; i < groupCount; i++) {
				if (groupUsage[i]) {
					for (uintptr_t j = 0; j < BITSET_GROUP_SIZE 
							&& groupUsage[i] 
							&& stackIndex != BITSET_STACK_SIZE; j++) {
						uintptr_t index = i * BITSET_GROUP_SIZE + j;

						if (singleUsage[index >> 5] & (1 << (index & 31))) {
							stack[stackIndex] = index;
							stackIndex++;

							singleUsage[index >> 5] &= ~(1 << (index & 31));
							groupUsage[i]--;
						}
					}

					goto returnFromStack;
				}
			}
		}
	} else if (count == 16) {
		for (uintptr_t i = 0; i < groupCount; i++) {
			if (groupUsage[i] >= 16) {
				for (uintptr_t j = 0; j < BITSET_GROUP_SIZE; j += 16) {
					uintptr_t index = i * BITSET_GROUP_SIZE + j;

					if (((uint16_t *) singleUsage)[index >> 4] == (uint16_t) (-1)) {
						((uint16_t *) singleUsage)[index >> 4] = 0;
						groupUsage[i] -= 16;
						returnValue = index;
						goto done;
					}
				}
			}
		}
	} else if (count == 32) {
		for (uintptr_t i = 0; i < groupCount; i++) {
			if (groupUsage[i] >= 32) {
				for (uintptr_t j = 0; j < BITSET_GROUP_SIZE; j += 32) {
					uintptr_t index = i * BITSET_GROUP_SIZE + j;

					if (singleUsage[index >> 5] == (uint32_t) (-1)) {
						singleUsage[index >> 5] = 0;
						groupUsage[i] -= 32;
						returnValue = index;
						goto done;
					}
				}
			}
		}
	}

	done:;
	return returnValue;
}

void Bitset::Put(uintptr_t index, bool intoBitset) {
	if (modCheck) KernelPanic("Bitset::Put - Concurrent modification.\n");
	modCheck = true; Defer({modCheck = false;});

	if (index > singleCount) {
		KernelPanic("Bitset::Put - Index greater than single code.\n");
	}

	if (singleUsage[index >> 5] & (1 << (index & 31))) {
		KernelPanic("Bitset::Put - Duplicate entry.\n");
	}

	if (stackIndex == BITSET_STACK_SIZE || intoBitset) {
		singleUsage[index >> 5] |= 1 << (index & 31);
		groupUsage[index / BITSET_GROUP_SIZE]++;
	} else {
		stack[stackIndex] = index;
		stackIndex++;
	}
}

#endif
