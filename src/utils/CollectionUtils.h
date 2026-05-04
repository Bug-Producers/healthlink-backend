#pragma once

/**
 * @brief Utilities for managing collections and data structures.
 */
namespace CollectionUtils {

    /**
     * @brief Clones a linked list of nodes and returns both head and tail.
     * @tparam NodeT The node type.
     * @tparam T The data type.
     * @param head The head of the list to clone.
     * @param outTail Reference to a pointer that will receive the new tail.
     * @return The new head of the cloned list.
     */
    template <typename NodeT>
    NodeT* cloneNodes(NodeT* head, NodeT** outTail = nullptr) {
        if (!head) {
            if (outTail) *outTail = nullptr;
            return nullptr;
        }

        NodeT* newHead = new NodeT(head->data);
        NodeT* currentNew = newHead;
        NodeT* currentOld = head->next;

        while (currentOld) {
            currentNew->next = new NodeT(currentOld->data);
            currentNew = currentNew->next;
            currentOld = currentOld->next;
        }

        if (outTail) *outTail = currentNew;
        return newHead;
    }
}
