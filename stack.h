/*****************************************************************************
 * Copyright (C) 2010  Tadeus Prastowo (eus@member.fsf.org)                  *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU General Public License as published by      *
 * the Free Software Foundation, either version 3 of the License, or         *
 * (at your option) any later version.                                       *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU General Public License for more details.                              *
 *                                                                           *
 * You should have received a copy of the GNU General Public License         *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *************************************************************************//**
 * @file stack.h
 * @brief A stack data structure.
 ****************************************************************************/

#ifndef STACK_H
#define STACK_H

#ifdef __cplusplus
extern "C" {
#endif

/** A stack of objects. */
typedef struct stack_impl stack;

/**
 * Creates a stack of objects. The created stack should be freed later with
 * destroy_stack().
 *
 * @param [out] s the resulting stack.
 * @param [in] element_size the size in bytes of every element in the stack.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
create_stack (stack **s, unsigned long element_size);

/**
 * Frees the memory allocated through create_stack()
 * and sets the pointer to NULL as a safe guard. Passing a pointer to NULL is
 * okay but not a NULL pointer.
 *
 * @param [in] s the stack to be freed.
 */
void
destroy_stack (stack **s);

/**
 * Pushes an object to the top of the stack. The object will be stored by doing
 * a memcpy().
 *
 * @param [in] e the object to push.
 * @param [in] s the stack to store the object.
 *
 * @return 0 if there is no error or non-zero if there is an error.
 */
int
push (void *e, stack *s);

/**
 * Pops an object from the top of the stack.
 *
 * @param [in] e a pointer to a space to contain the popped object (pass NULL
 *               to not store the popped object).
 * @param [in] s the stack to be popped.
 *
 * @return 0 if there is no error or -1 if the stack is empty or other non-zero
 *         if there is an error.
 */
int
pop (void *e, stack *s);

/**
 * Reads the top element of a stack.
 *
 * @param [in] e a pointer to a space to contain the read object.
 * @param [in] s the stack to be read.
 *
 * @return 0 if there is no error or -1 if the stack is empty or other non-zero
 *         if there is an error.
 */
int
top (void *e, const stack *s);

/**
 * Checks whether or not the stack is empty.
 *
 * @param [in] s the stack to be checked.
 *
 * @return 0 if the stack is not empty or non-zero if the stack is.
 */
int
is_empty (const stack *s);

#ifdef __cplusplus
}
#endif

#endif /* STACK_H */
