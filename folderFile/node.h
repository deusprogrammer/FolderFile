#ifndef NODE_H
#define NODE_H

#define NULL 0

template <class T>
struct Node {
	Node<T> *next;
	Node<T> *prev;
	T data;
	Node() {next = NULL; prev = NULL;}
};

#endif