/******************************************************************************
  Copyright (c) 2013 Morten Houmøller Nygaard - www.mortz.dk - admin@mortz.dk
 
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include "Datastructures.h"

/**
 * Returns a new list structure.
 */
struct list *list_new (void) {
	struct list *l;

	l = (struct list *) malloc(sizeof(struct list));
	
	if (l != NULL) {
		l->len = 0;
		l->first = l->last = NULL;

		pthread_mutex_init(&l->lock, NULL);	
	} else {
		exit(EXIT_FAILURE);
	}

	return l;
}

/**
 * Frees the list structure, including all its nodes.
 */
void list_free (struct list *l) {
	struct node *n = l->first, *p;
	pthread_mutex_lock(&l->lock);
	
	while (n != NULL) {
		p = n->next;

		node_free(n);

		close(n->socket_id);
		free(n);
		n = p;
	}

	l->first = l->last = NULL;
	pthread_mutex_unlock(&l->lock);	
	pthread_mutex_destroy(&l->lock);
	free(l);
}

/**
 * Adds a node to the list l.
 */
void list_add (struct list *l, struct node *n) {
	pthread_mutex_lock(&l->lock);
	
	if (l->first != NULL) {
		l->last = l->last->next = n;	
	} else {
		l->first = l->last = n;	
	}

	l->len++;
	
	pthread_mutex_unlock(&l->lock);
}

/**
 * Removes a node from the list l
 */
void list_remove (struct list *l, struct node *r) {
	struct node *n = l->first, *p;
	pthread_mutex_lock(&l->lock);

	if (n == NULL) {
		pthread_mutex_unlock(&l->lock);
		return;
	}

	do {
		if (n == r) {

			if (n == l->first) {
				l->first = n->next;
			} else {
				p->next = n->next;
			}

			if (n == l->last) {
				l->last = p;
			}

			node_free(n);

			close(n->socket_id);
			free(n);
			l->len--;
			break;
		}
		
		p = n;
		n = n->next;
	} while (n != NULL); 

	if (l->len == 0) {
		l->first = l->last = NULL;
	} else if (l->len == 1) {
		l->last = l->first;
	}

	pthread_mutex_unlock(&l->lock);
}

void list_remove_all (struct list *l) {
	struct node *n = l->first;
	char close[2];
	pthread_mutex_lock(&l->lock);

	if (n == NULL) {
		pthread_mutex_unlock(&l->lock);
		return;
	}

	close[0] = '\x88';
	close[1] = '\x00';

	do {
		send(n->socket_id, close, 2, 0);
		
		n = n->next;
	} while (n != NULL); 

	pthread_mutex_unlock(&l->lock);
}

void list_delete(struct list *l, struct node *r) {
	struct node *n = l->first, *p;
	pthread_mutex_lock(&l->lock);

	if (n == NULL) {
		pthread_mutex_unlock(&l->lock);
		return;
	}

	do {
		if (n == r) {

			if (n == l->first) {
				l->first = n->next;
			} else {
				p->next = n->next;
			}

			if (n == l->last) {
				l->last = p;
			}

			l->len--;
			break;
		}

		p = n;
		n = n->next;
	} while (n != NULL); 

	if (l->len == 0) {
		l->first = l->last = NULL;
	} else if (l->len == 1) {
		l->last = l->first;
	}

	pthread_mutex_unlock(&l->lock);
}

/**
 * Prints out information about each node contained in the list.
 */
void list_print(struct list *l) {
	struct node *n = l->first;
	pthread_mutex_lock(&l->lock);

	if (n == NULL) {
		printf("No clients are online.\n\n");
		fflush(stdout);
		pthread_mutex_unlock(&l->lock);
		return;
	}

	do {
		printf("Socket Id: \t\t%d\n"
			   "Client IP: \t\t%s\n",
			   n->socket_id, n->client_ip);
		fflush(stdout);
		n = n->next;
	} while (n != NULL);
	pthread_mutex_unlock(&l->lock);
}

void list_multicast(struct list *l, struct node *n) {
	struct node *p = l->first;
	pthread_mutex_lock(&l->lock);
	
	if (p == NULL) {
		pthread_mutex_unlock(&l->lock);
		return;
	}

	do {
		if (p != n) {
			send(p->socket_id, n->message->enc, n->message->enc_len, 0);
		}
		p = p->next;
	} while (p != NULL);
	pthread_mutex_unlock(&l->lock);
}

void list_multicast_one(struct list *l, struct node *n, struct message *m) {
	struct node *p = l->first;
	pthread_mutex_lock(&l->lock);
	
	if (p == NULL) {
		pthread_mutex_unlock(&l->lock);
		return;
	}

	do {
		if (p == n) {
			send(p->socket_id, m->enc, m->enc_len, 0);
			break;
		}
		p = p->next;
	} while (p != NULL);
	pthread_mutex_unlock(&l->lock);
}

void list_multicast_all(struct list *l, struct message *m) {
	struct node *p = l->first;
	pthread_mutex_lock(&l->lock);
	
	if (p == NULL) {
		pthread_mutex_unlock(&l->lock);
		return;
	}

	do {
		send(p->socket_id, m->enc, m->enc_len, 0);
		p = p->next;
	} while (p != NULL);
	pthread_mutex_unlock(&l->lock);
}

struct node *list_get(struct list *l, char *addr, int socket) {
	struct node *p = l->first;
	pthread_mutex_lock(&l->lock);
	
	if (p == NULL) {
		pthread_mutex_unlock(&l->lock);
		return p;
	}

	do {
		if (p->socket_id == socket && strcmp(addr, p->client_ip) == 0) {
			break;
		}
		p = p->next;
	} while (p != NULL);
	pthread_mutex_unlock(&l->lock);

	return p;
}

/**
 * Creates a new node.
 */
struct node *node_new (int sock, char *addr) {
	struct node *n;

	n = (struct node *) malloc(sizeof(struct node));

	if (n != NULL) {
		n->socket_id = sock;
		n->thread_id = 0;
		n->client_ip = addr;
		n->string = NULL;
		n->headers = NULL;
		n->next = NULL;
		n->message = NULL;
	} else {
		exit(EXIT_FAILURE);
	}

	return n;
}

struct header *header_new () {
	struct header *h;

	h = (struct header *) malloc(sizeof(struct header));

	if (h != NULL) {
		h->host = NULL;
		h->connection = NULL;
		h->key = NULL;
		h->key1 = NULL;
		h->key2 = NULL;
		h->key3 = NULL;
		h->version = NULL;
		h->type = NULL;
		h->protocol = NULL;
		h->origin = NULL;
		h->upgrade = NULL;
		h->get = NULL;
		h->accept = NULL;
		h->extension = NULL;
	} else {
		exit(EXIT_FAILURE);
	}

	return h;
}

struct message *message_new() {
	struct message *m;

	m = (struct message *) malloc(sizeof(struct message));

	if (m != NULL) {
		memset(m->opcode, '\0', 1);
		memset(m->mask, '\0', 4);
		m->len = 0; 
		m->enc_len = 0;
		m->next_len = 0;
		m->msg = NULL;
		m->next = NULL;
		m->enc = NULL;
	} else {
		exit(EXIT_FAILURE);
	}

	return m;	
}

void header_free(struct header *h) {
	if (h->accept != NULL) {
		free(h->accept);
		h->accept = NULL;
	}
}

void message_free(struct message *m) {
	if (m->msg != NULL) {
		free(m->msg);
		m->msg = NULL;
	}
	
	if (m->enc != NULL) {
		free(m->enc);
		m->enc = NULL;
	}

	if (m->next != NULL) {
		free(m->next);
		m->next = NULL;
	}
}

void node_free(struct node *n) {
	if (n->client_ip != NULL) {
		free(n->client_ip);
		n->client_ip = NULL;
	}

	if (n->string != NULL) {
		free(n->string);
		n->string = NULL;
	}

	if (n->headers != NULL) {
		header_free(n->headers);
		free(n->headers);
		n->headers = NULL;
	}

	if (n->message != NULL) {
		message_free(n->message);
		free(n->message);
		n->message = NULL;
	}
}