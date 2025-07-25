/*
MIT License

Copyright (c) 2021 Mikel Irazabal

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>

#include "bimap.h"
#include "assoc_generic.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void bi_map_init(bi_map_t* map, size_t key_sz1, size_t key_sz2, bi_map_cmp cmp1, bi_map_cmp cmp2, free_func_t free1, free_func_t free2)
{
  assert(map != NULL);
  assert(key_sz1 > 0);
  assert(key_sz2 > 0);
  assert(cmp1 != NULL);
  assert(cmp2 != NULL);

  assoc_init(&map->left, key_sz1, cmp1, free1);
  assoc_init(&map->right, key_sz2, cmp2, free2);
}

void bi_map_free(bi_map_t* map)
{
  assert(map != NULL);

  assoc_free(&map->left);
  assoc_free(&map->right);
}


// Modifiers

void bi_map_insert(bi_map_t* map, void const* key1, size_t key_sz1, void const* key2, size_t key_sz2)
{
  assert(map != NULL);
  assert(key1 != NULL);
  assert(key_sz1 > 0);
  assert(key2 != NULL);
  assert(key_sz2 > 0);

  void* val1 = malloc(key_sz2);
  assert(val1 != NULL && "Memory exhausted");
  
  // POD or this is a bug. If it is not, it will probably be in the future
  memcpy(val1, key2, key_sz2);

  // assert that they are equal
  int cmp = map->right.comp(key2, val1);
  assert(cmp == 0 );

  //printf("bi_map_insert: inserting into left rb_tree (key=%d)\n", *(int*)key1);
  assoc_insert(&map->left, key1, key_sz1, val1);

  void* val2 = malloc(key_sz1);
  assert(val2 != NULL && "Memory exhausted");

  // POD or this is a bug. If it is not, it will probably be in the future
  memcpy(val2, key1, key_sz1);

  // assert that they are equal
  cmp = map->left.comp(key1, val2);
  assert(cmp == 0 );

  //printf("bi_map_insert: inserting into right rb_tree (value=%d)\n", *(int*)val2);
  assoc_insert(&map->right, key2, key_sz2, val2);
}

// It returns the void* of key2. the void* of the key1 is freed
void* bi_map_extract_left(bi_map_t* map, void* key1, size_t key1_sz, free_fp_key f)
{
  assert(map != NULL);
  assert(key1 != NULL);
  assert(key1_sz == map->left.key_sz);


  //printf("bi_map_extract_left: about to call assoc_extract\n");
  void* key2 = assoc_extract(&map->left, key1);
  void* key3 = assoc_extract(&map->right, key2);

  int cmp = map->left.comp(key1, key3);
  // I do not like this trick. The memory should also be the same
  //int cmp = memcmp(key1, key3, map->right.key_sz);
  assert(cmp == 0);

  if(f != NULL)
    f(key3);
  free(key3);

  size_t sz_1 = assoc_size(&map->left);
  size_t sz_2 = assoc_size(&map->right);

  assert(sz_1 == sz_2);

  return key2;
}

// It returns the void* of key1. the void* of the key2 is freed
void* bi_map_extract_right(bi_map_t* map, void* key2, size_t key2_sz, free_fp_key f)
{
  assert(map != NULL);
  assert(key2 != NULL);
  assert(key2_sz == map->right.key_sz);

  void* key1 = assoc_extract(&map->right, key2);
  //printf("bi_map_extract_right: assoc_extract removed ev=%p\n",key1);
  void* key3 = assoc_extract(&map->left, key1);
  //printf("bi_map_extract_right: assoc_extract removed fd=%p\n",key2);

  int cmp = map->right.comp(key2, key3);
  assert(cmp == 0);

  assert(map->right.free_func != NULL);

  if(f != NULL)
    f(key3);
  free(key3);

  size_t sz_1 = assoc_size(&map->left);
  size_t sz_2 = assoc_size(&map->right);

  assert(sz_1 == sz_2);

  return key1;
}

// Capacity
size_t bi_map_size(bi_map_t* map)
{
  assert(map != NULL);
  size_t cap = assoc_size(&map->left);
  assert(cap == assoc_size(&map->right) && "Mismatch in the ds");
  return cap;
}


// Forward Iterator Concept

bml_iter_t bi_map_front_left(bi_map_t* map)
{
  assert(map != NULL);
  bml_iter_t it = {.it = assoc_front(&map->left)};
  return it;
}

bml_iter_t bi_map_next_left(bi_map_t* map, bml_iter_t it)
{
  assert(map != NULL);
  assert(it.it != NULL);
  bml_iter_t it_next = {.it = assoc_next(&map->left, it.it)};
  return it_next;
}

bml_iter_t bi_map_end_left(bi_map_t* map)
{
  assert(map != NULL);
  bml_iter_t it = {.it = assoc_end(&map->left)};
  return it;
}

bmr_iter_t bi_map_front_right(bi_map_t* map)
{
  assert(map != NULL);
  bmr_iter_t it = {.it = assoc_front(&map->right)};
  return it;
}

bmr_iter_t bi_map_next_right(bi_map_t* map, bmr_iter_t it)
{
  assert(map != NULL);
  assert(it.it != NULL);
  bmr_iter_t it_next = {.it = assoc_next(&map->right, it.it)};
  return it_next;
}

bmr_iter_t bi_map_end_right(bi_map_t* map)
{
  assert(map != NULL);
  bmr_iter_t it = {.it = assoc_end(&map->right)};
  return it;
}

void* bi_map_value_left(bi_map_t* map, bml_iter_t it)
{
  assert(map != NULL);
  return assoc_value(&map->left, it.it); 
}

void* bi_map_value_right(bi_map_t* map, bml_iter_t it)
{
  assert(map != NULL);
  return assoc_value(&map->right, it.it); 
}

