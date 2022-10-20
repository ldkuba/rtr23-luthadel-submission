#pragma once

template<class T>
class SinglyLinkedList {
  public:
    struct Node {
        T     data;
        Node* next;
    };

    Node* head;

  public:
    SinglyLinkedList();

    void insert(Node* previous_node, Node* new_node);
    void remove(Node* previous_node, Node* delete_node);
};

template<class T>
SinglyLinkedList<T>::SinglyLinkedList() {}

template<class T>
void SinglyLinkedList<T>::insert(Node* previous_node, Node* new_node) {
    // Is the first node?
    if (previous_node == nullptr) {
        new_node->next = head;
        head           = new_node;
        return;
    }

    new_node->next      = previous_node->next;
    previous_node->next = new_node;
}

template<class T>
void SinglyLinkedList<T>::remove(Node* previous_node, Node* delete_node) {
    // Is the first node?
    if (previous_node == nullptr) {
        head = delete_node->next;
        return;
    }
    previous_node->next = delete_node->next;
}