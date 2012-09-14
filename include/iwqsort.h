/**************************************************************************

    Copyright (C) 2011  Eli Lilly and Company

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************/
#ifndef IWQSORT_H
#define IWQSORT_H 1

template <typename T> void iwqsort (T *, int);
template <typename T> void iwqsort (T *, int, int (*) (T &, T &) );
template <typename T> void iwqsort (T *, int, int (*) (const T &, const T &) );

// With a function object comparitor

template <typename T, typename C> void iwqsort (T *, int, C &);

#if (IW_IMPLEMENTATIONS_EXPOSED) || defined(RESIZABLE_ARRAY_IWQSORT_IMPLEMENTATION)
#define IWQSORT_FO_IMPLEMENTATION

#include "iwaray.h"

template <typename T> template <typename C>
void
resizable_array_base<T>::iwqsort (C & comparator)
{
  if (_number_elements < 2)
    return;

  ::iwqsort (_things, _number_elements, comparator);

  return;
}

#endif

#if (IW_IMPLEMENTATIONS_EXPOSED) || defined(IWQSORT_FO_IMPLEMENTATION)

#include <assert.h>

//#define DEBUG_IWQSORT

template <typename T>
void 
swap_elements (T & t1, T & t2,
               void * tmp)
{
//cerr << "Swapping " << t1 << " and " << t2 << ", size " << sizeof (T) << endl;
#ifdef IWQS_USE_OPERATOR
  *tmp = t1;
  t1 = t2;
  t2 = *tmp;
#else
  memcpy (tmp, &t1, sizeof (T));
  memcpy (&t1, &t2, sizeof (T));
  memcpy (&t2, tmp, sizeof (T));
#endif

  return;
}

template <typename T, typename C>
void
compare_two_items (T * t,
                   C & comparitor,
                   void * tmp)
{
  int c = comparitor (t[0], t[1]);

  if (c <= 0)
    return;

  swap_elements (t[0], t[1], tmp);

  return;
}

/*
  Scan left until we find something less than pivot
*/

template <typename T, typename C>
void
move_in_from_right (T * t, 
                    int & low,
                    int & high,
                    C & comparitor)
{
  T & pivot = t[low];

  while (high > low)
  {
    int c = comparitor (pivot, t[high]);
#ifdef DEBUG_IWQSORT
    cerr << "Right between " << pivot << " and " << t[high] << " comparison " << c << endl;
#endif

    if (c <= 0)
      high--;
    else if (c > 0)
      break;
  }

  return;
}

/*
  Scan right until we find something greater than pivot
*/

template <typename T, typename C>
void
move_in_from_left (T * t,
                   int & low,
                   int & left,
                   int n,
                   C & comparitor,
                   void * tmp)
{
  T & pivot = t[low];

  while (left < n)
  {
    int c = comparitor (pivot, t[left]);
#ifdef DEBUG_IWQSORT
    cerr << "Left " << left << " between " << pivot << " and " << t[left] << " comparison " << c << endl;
#endif

    if (c > 0)
      left++;
    else if (0 == c)
    {
      low++;
      if (left > low)
        swap_elements (t[low], t[left], tmp);
      left++;
    }
    else
      break;
  }

  return;
}

template <typename T, typename C>
void
iwqsort (T * t, int n,
         C & comparitor,
         void * tmp)
{
#ifdef DEBUG_IWQSORT
  cerr << "On entry n = " << n << endl;
  for (int i = 0; i < n; i++)
  {
    cerr << " i = " << i << " value " << t[i] << endl;
  }
#endif

  if (n < 1)
    return;

  if (2 == n)
  {
    compare_two_items (t, comparitor, tmp);
    return;
  }

  int low = 0;
  int left = 1;
  int right = n - 1;
  while (1)
  {
    move_in_from_left (t, low, left, n, comparitor, tmp);
    move_in_from_right (t, low, right, comparitor);

//  cerr << "Low " << low << " Left " << left << " right " << right << endl;
    if (left < right)
      swap_elements (t[left], t[right], tmp);
    else
      break;
  }

  if (right > low)
  {
    if (low == n - 1)     // all values in this chunk constant
      return;

#ifdef DEBUG_IWQSORT
    cerr << "N = " << n << " before moving, low = " << low << " left " << left << " right " << right << endl;
    for (int i = 0; i < n; i++)
    {
      cerr << " i " << i << " value " << t[i] << endl;
    }
#endif

    for (int i = 0; i < low + 1; i++)
    {
      swap_elements (t[i], t[right - i], tmp);
    }
  }

#ifdef DEBUG_IWQSORT
  cerr << "Starting recursion, low " << low << " left " << left << " right " << right << endl;
  for (int i = 0; i < n; i++)
  {
    cerr << " i " << i << " value " << t[i] << endl;
  }
#endif

  iwqsort (t, right - low, comparitor, tmp);
  iwqsort (t + right + 1, n - right - 1, comparitor, tmp);

  return;
}

template <typename T, typename C>
void
iwqsort (T * t, int n,
         C & comparitor)
{
  assert (n >= 0);

  if (n < 2)
    return;

  unsigned char * tmp = new unsigned char[sizeof (T)];

  iwqsort (t, n, comparitor, tmp);

  delete tmp;

  return;
}

#endif


#if (IW_IMPLEMENTATIONS_EXPOSED) || defined(IW_IMPLEMENTATIONS_EXPOSED)

#include <assert.h>

#define IWSORT_FN iwqsort
#define IWDEREF(o) o.

//#define DEBUG_IWQSORT

template <typename T>
void 
swap_elements (T * t1, T * t2)
{
  T tmp;
  memcpy (&tmp, t1, sizeof (T));
  memcpy (t1, t2, sizeof (T));
  memcpy (t2, &tmp, sizeof (T));

  return;
}

template <typename T>
void
#ifdef IWQS_USER_FUNCTION_SUPPLIED
compare_two_items (T * t,
                   int (*mfn) (T &, T &))
{
  int c = mfn (t[0], t[1]);

  if (c <= 0)
    return;

  swap_elements(t, t+ 1);

  return;
}
#elif defined(IWQS_USE_OPERATOR)
compare_two_items (T * t)
{
  if (t[0] <= t[1])
    return;

  swap_elements(t, t + 1);

  return;
}
#else
compare_two_items (T * t)
{
  int c = IWDEREF(t[0])iwqsortcompare (t[1]);

  if (c <= 0)
    return;

  swap_elements (t, t + 1);

  return;
}
#endif

/*
  Scan left until we find something less than pivot
*/

template <typename T>
void
#ifdef IWQS_USER_FUNCTION_SUPPLIED
move_in_from_right (T * t, 
                    int low,
                    int & high,
                    int (*mfn) (T &, T &))
#else
move_in_from_right (T * t, 
                    int low,
                    int & high)
#endif
{
  T & pivot = t[low];

  while (high > low)
  {
#ifdef IWQS_USER_FUNCTION_SUPPLIED
    int c = mfn (pivot, t[high]);
#elif defined(IWQS_USE_OPERATOR)
    int c;
    if (pivot <= t[high])
      c = 0;
    else
      c = 1;
#else
    int c = pivot.iwqsortcompare (t[high]);
#endif
//  cerr << "Right between " << IWDEREF(pivot)zvalue () << " and " << IWDEREF(t[high])zvalue () << " comparison " << c << endl;
//  cerr << "Right between " << pivot << " and " << t[high] << " comparison " << c << endl;

    if (c <= 0)
      high--;
    else if (c > 0)
      break;
  }

  return;
}

/*
  Scan right until we find something greater than pivot
*/

template <typename T>
void
#ifdef IWQS_USER_FUNCTION_SUPPLIED
move_in_from_left (T * t,
                   int & low,
                   int & left,
                   int n,
                   int (*mfn) (T &, T &))
#else
move_in_from_left (T * t,
                   int & low,
                   int & left,
                   int n)
#endif
{
  T & pivot = t[low];

  while (left < n)
  {
#ifdef IWQS_USER_FUNCTION_SUPPLIED
    int c = mfn (pivot, t[left]);
#elif defined(IWQS_USE_OPERATOR)
    int c;
    if (pivot == t[left])
      c = 0;
    else if (pivot < t[left])
      c = -1;
    else
      c = 1;
#else
    int c = pivot.iwqsortcompare (t[left]);
#endif
//  cerr << "Left " << left << " between " << IWDEREF(pivot)zvalue () << " and " << IWDEREF(t[left])zvalue () << " comparison " << c << endl;

    if (c > 0)
      left++;
    else if (0 == c)
    {
      low++;
      if (left > low)
        swap_elements (t + low, t + left);
      left++;
    }
    else
      break;
  }

  return;
}

template <typename T>
void
#ifdef IWQS_USER_FUNCTION_SUPPLIED
IWSORT_FN (T * t, int n,
         int (*mfn) (T &, T &))
#else
IWSORT_FN (T * t, int n)
#endif
{
#ifdef DEBUG_IWQSORT
  cerr << "On entry n = " << n << endl;
  for (int i = 0; i < n; i++)
  {
#ifdef IWQS_USE_OPERATOR
    cerr << " i = " << i << " value " << t[i] << endl;
#else
    cerr << " i = " << i << " value " << IWDEREF(t[i])zvalue () << endl;
#endif
  }
#endif
  assert (n >= 0);

  if (n < 2)
    return;

  if (2 == n)
  {
#ifdef IWQS_USER_FUNCTION_SUPPLIED
    compare_two_items (t, mfn);
#else
    compare_two_items (t);
#endif
    return;
  }

  int low = 0;
  int left = 1;
  int right = n - 1;
  while (1)
  {
#ifdef IWQS_USER_FUNCTION_SUPPLIED
    move_in_from_left (t, low, left, n, mfn);
    move_in_from_right (t, low, right, mfn);
#else
    move_in_from_left (t, low, left, n);
    move_in_from_right (t, low, right);
#endif

//  cerr << "Low " << low << " Left " << left << " right " << right << endl;
    if (left < right)
      swap_elements (t + left, t + right);
    else
      break;
  }

  if (right > low)
  {
    if (low == n - 1)     // all values in this chunk constant
      return;

#ifdef DEBUG_IWQSORT
    cerr << "N = " << n << " before moving, low = " << low << " left " << left << " right " << right << endl;
    for (int i = 0; i < n; i++)
    {
#ifdef IWQS_USE_OPERATOR
      cerr << " i " << i << " value " << t[i] << endl;
#else
      cerr << " i " << i << " value " << IWDEREF(t[i])zvalue () << endl;
#endif
    }
#endif

    for (int i = 0; i < low + 1; i++)
    {
      swap_elements (t + i, t + right - i);
    }
  }

#ifdef DEBUG_IWQSORT
  cerr << "Starting recursion, low " << low << " left " << left << " right " << right << endl;
  for (int i = 0; i < n; i++)
  {
#ifdef IWQS_USE_OPERATOR
    cerr << " i " << i << " value " << t[i] << endl;
#else
    cerr << " i " << i << " value " << IWDEREF(t[i])zvalue () << endl;
#endif
  }
#endif

#ifdef IWQS_USER_FUNCTION_SUPPLIED
  IWSORT_FN (t, right - low, mfn);
  IWSORT_FN (t + right + 1, n - right - 1, mfn);
#else
  IWSORT_FN (t, right - low);
  IWSORT_FN (t + right + 1, n - right - 1);
#endif

  return;
}

#endif
#endif
