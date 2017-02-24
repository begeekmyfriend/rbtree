/*
 * Copyright (C) 2015, Leo Ma <begeekmyfriend@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "rbtree.h"

#define N                  64//1024 * 1024
#define RBTREE_MAX_LEVEL   64
#define RBTREE_LEFT_INDEX  0
#define RBTREE_RIGHT_INDEX 1
// #define RBTREE_DEBUG
#define RBTREE_DUMP

struct node_backlog {
        /* Node backlogged */
        struct rbnode *node;
        /* The index next to the backtrack point, valid when >= 1 */
        int next_sub_idx;
};

static inline void
nbl_push(struct node_backlog *nbl, struct node_backlog **top, struct node_backlog **bottom)
{
        if (*top - *bottom < RBTREE_MAX_LEVEL) {
                (*(*top)++) = *nbl;
        }
}

static inline struct node_backlog *
nbl_pop(struct node_backlog **top, struct node_backlog **bottom)
{
        return *top > *bottom ? --*top : NULL;
}

static inline int
nbl_is_empty(struct node_backlog *top, struct node_backlog *bottom)
{
        return top == bottom;
}

static inline int
is_leaf(struct rbnode *node, struct rbnode *sentinel)
{
        return node->left == sentinel && node->right == sentinel;
}

static void
node_dump(struct rbnode *node)
{
        if (node != NULL) {
                struct rbnode *parent = node->parent;
                if (parent != NULL) {
                        if (node == parent->left) {
                                putchar('L');
                        } else if (node == parent->right) {
                                putchar('R');
                        }
                }
                if (rbtree_is_red(node)) {
                        printf("R:0x%08x\n", node->key);
                } else {
                        printf("B:0x%08x\n", node->key);
                }
        }
}

static void
rbtree_dump(struct rbtree *tree)
{
        int level = 0;
        struct rbnode *node = tree->root, *sentinel = tree->sentinel;
        struct node_backlog nbl, *p_nbl = NULL;
        struct node_backlog *top, *bottom, nbl_stack[RBTREE_MAX_LEVEL];

        top = bottom = nbl_stack;

        for (; ;) {
                if (node != sentinel) {
                        /* Fetch the pop-up backlogged node's sub-id.
                         * If not backlogged, fetch the first sub-id. */
                        int sub_index = p_nbl != NULL ? p_nbl->next_sub_idx : RBTREE_LEFT_INDEX;

                        /* backlog should be reset since node has gone deep down */
                        p_nbl = NULL;

                        /* Backlog the node */
                        if (is_leaf(node, sentinel) || sub_index == RBTREE_RIGHT_INDEX) {
                                nbl.node = sentinel;
                                nbl.next_sub_idx = RBTREE_LEFT_INDEX;
                        } else {
                                nbl.node = node;
                                nbl.next_sub_idx = RBTREE_RIGHT_INDEX;
                        }
                        nbl_push(&nbl, &top, &bottom);
                        level++;

                        /* Draw lines as long as sub_idx is the first one */
                        if (sub_index == RBTREE_LEFT_INDEX) {
                                int i;
                                for (i = 1; i < level; i++) {
                                        if (i == level - 1) {
                                                printf("%-8s", "+-------");
                                        } else {
                                                if (nbl_stack[i - 1].node != sentinel) {
                                                        printf("%-8s", "|");
                                                } else {
                                                        printf("%-8s", " ");
                                                }
                                        }
                                }
                                node_dump(node);
                        }

                        /* Move down according to sub_idx */
                        node = sub_index == RBTREE_LEFT_INDEX ? node->left : node->right;
                } else {
                        p_nbl = nbl_pop(&top, &bottom);
                        if (p_nbl == NULL) {
                                /* End of traversal */
                                break;
                        }
                        node = p_nbl->node;
                        level--;
                }
        }
}

static void
rbtree_dump_for_watch(struct rbtree *tree)
{
        int level = 0;
        struct rbnode *node = tree->root, *sentinel = tree->sentinel;
        struct node_backlog nbl, *p_nbl = NULL;
        struct node_backlog *top, *bottom, nbl_stack[RBTREE_MAX_LEVEL];

        top = bottom = nbl_stack;

        for (; ;) {
                if (node != sentinel) {
                        /* Fetch the pop-up backlogged node's sub-id.
                         * If not backlogged, fetch the first sub-id. */
                        int sub_index = p_nbl != NULL ? p_nbl->next_sub_idx : RBTREE_RIGHT_INDEX;

                        /* backlog should be reset since node has gone deep down */
                        p_nbl = NULL;

                        /* Backlog the node */
                        if (is_leaf(node, sentinel) || sub_index == RBTREE_LEFT_INDEX) {
                                nbl.node = sentinel;
                                nbl.next_sub_idx = RBTREE_RIGHT_INDEX;
                        } else {
                                nbl.node = node;
                                nbl.next_sub_idx = RBTREE_LEFT_INDEX;
                        }
                        nbl_push(&nbl, &top, &bottom);
                        level++;

                        /* Draw lines as long as sub_idx is the first one */
                        if (sub_index == RBTREE_RIGHT_INDEX) {
                                int i;
                                for (i = 1; i < level; i++) {
                                        if (i == level - 1) {
                                                printf("%-8s", "+-------");
                                        } else {
                                                if (nbl_stack[i - 1].node != sentinel) {
                                                        printf("%-8s", "|");
                                                } else {
                                                        printf("%-8s", " ");
                                                }
                                        }
                                }
                                node_dump(node);
                        }

                        /* Move down according to sub_idx */
                        node = sub_index == RBTREE_LEFT_INDEX ? node->left : node->right;
                } else {
                        p_nbl = nbl_pop(&top, &bottom);
                        if (p_nbl == NULL) {
                                /* End of traversal */
                                break;
                        }
                        node = p_nbl->node;
                        level--;
                }
        }
}

int
main(void)
{
        int i, *key;
        struct timespec start, end;
        struct rbtree tree;
        struct rbnode sentinel, *min, *max, *node;

        key = malloc(N * sizeof(int));
        if (key == NULL) {
                exit(-1);
        }

        rbtree_init(&tree, &sentinel);

        /* Insert test */
        printf("Add %d nodes...\n", N);
        srandom(time(NULL));
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (i = 0; i < N; i++) {
                struct rbnode *n = malloc(sizeof(*n));
                if (n == NULL) {
                        exit(-1);
                }
                n->key = n->data = key[i] = random();
                rbtree_insert(&tree, n);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("time span: %ldms\n", (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1000000);

        /* Min and max */
        min = rbtree_min(&tree);
        max = rbtree_max(&tree);
        printf("min:0x%08x\n", min->key);
        printf("max:0x%08x\n", max->key);

#ifdef RBTREE_DUMP
      rbtree_dump_for_watch(&tree);
#endif
        /* Min and max */
        printf("min:0x%08x\n", min->key);
        printf("max:0x%08x\n", max->key);

        /* ordered traversal */
        printf("ordered traversal...\n");
        for (node = min; node != max; node = rbtree_next(&tree, node)) {
#ifdef RBTREE_DEBUG
                printf("key:0x%08x value:0x%08x\n", node->key, node->data);
#endif
        }

        /* reversed traversal */
        printf("reversed traversal...\n");
        for (node = max; node != min; node = rbtree_prev(&tree, node)) {
#ifdef RBTREE_DEBUG
                printf("key:0x%08x value:0x%08x\n", node->key, node->data);
#endif
        }

        /* Search test */
        printf("Search each node...\n");
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (i = 0; i < N; i++) {
                struct rbnode *n = rbtree_search(&tree, key[i]);
#ifdef RBTREE_DEBUG
                printf("key:0x%08x value:0x%08x\n", n->key, n->data);
#endif
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("time span: %ldms\n", (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1000000);

        /* Remove test */
        printf("Remove all nodes...\n");
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (i = 0; i < N; i++) {
                struct rbnode *n = rbtree_search(&tree, key[i]);
                rbtree_delete(&tree, n);
                free(n);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        printf("time span: %ldms\n", (end.tv_sec - start.tv_sec)*1000 + (end.tv_nsec - start.tv_nsec)/1000000);

        return 0;
}
