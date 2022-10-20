#pragma once

template<class T>
class StackLinkedList {
  public:
    struct Node {
        T     data;
        Node* next;
    };

    Node* head;

    StackLinkedList()                                   = default;
    StackLinkedList(StackLinkedList& stack_linked_list) = delete;

    void  push(Node* new_node);
    Node* pop();
};

template<class T>
void StackLinkedList<T>::push(Node* new_node) {
    new_node->next = head;
    head           = new_node;
}

template<class T>
typename StackLinkedList<T>::Node* StackLinkedList<T>::pop() {
    Node* top = head;
    head      = head->next;
    return top;
}