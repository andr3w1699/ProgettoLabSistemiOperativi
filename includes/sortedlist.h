// Linked List in C Implementation code
// with a function insertion_sort to insert a node in the list in a sorted way in ascending order
// using the long data node field as the key to sort

#include <stdio.h>
#include <stdlib.h>

// a node in linked list
struct Node
{
  long data;         // result
  char *file_name;   // filename
  struct Node *next; // Pointer pointing towards next node
};

// function to print the linked list
void printList(struct Node *node)
{
  while (node != NULL)
  {
    printf("%ld %s \n", node->data, node->file_name);
    node = node->next;
  }
}

// create a new Node with data and file_name fields as the args
struct Node *newNode(long data, char *filename)
{
  struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
  newNode->data = data;
  newNode->file_name = (char *)malloc(sizeof(char) * (strlen(filename) + 1));
  strcpy(newNode->file_name, filename);
  newNode->next = NULL;
  return newNode;
}

// function to insert data in sorted position (ascendind order, key is result field)
void insertion_sort(struct Node **head, struct Node *newNode)
{
  // If linked list is empty or we have to perform an insertion in the head of the list
  if (*head == NULL || (*head)->data >= newNode->data)
  { // inserimento in testa
    newNode->next = *head;
    *head = newNode;
    return;
  }

  // Locate the node before insertion
  struct Node *current = *head;
  while (current->next != NULL && current->next->data < newNode->data)
    current = current->next;

  newNode->next = current->next;
  current->next = newNode;
}

// function to free the memory allocated for the list
void free_list(struct Node *head)
{
  struct Node *current = head;
  struct Node *next;
  while (current != NULL)
  {
    next = current->next;     // Save the next node
    free(current->file_name); // free file_name field memory
    free(current);            // Free the current node
    current = next;           // Move to the next node
  }
}