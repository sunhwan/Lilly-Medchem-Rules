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
#include "iwstring.h"
#include "smiles.h"
#include "iwrnm.h"
#include "misc.h"
#include "misc2.h"

#ifdef IW_USE_TBB_SCALABLE_ALLOCATOR
#include "tbb/scalable_allocator.h"
#endif

static int FileScope_include_directionality_in_ring_closure_bonds = 0;

int
include_directionality_in_ring_closure_bonds ()
{
  return FileScope_include_directionality_in_ring_closure_bonds;
}

void
set_include_directionality_in_ring_closure_bonds (int s)
{
  FileScope_include_directionality_in_ring_closure_bonds = s;
}

void
Ring_Number_Manager::_default_values ()
{
  _nr = 0;
  _ring_id = NULL;
  _bond = NULL;
//_bt = NULL;
  _from_atom = NULL;
  _include_aromaticity_in_smiles = get_include_aromaticity_in_smiles();
  _include_cis_trans_in_smiles = include_cis_trans_in_smiles();

  return;
}

Ring_Number_Manager::Ring_Number_Manager ()
{
  _default_values ();
}

Ring_Number_Manager::Ring_Number_Manager (int nrings)
{
  _default_values ();

  if (0 == nrings)
  {
    return;
  }

  activate (nrings + 1);      // we don't use ring number 0, so allocate space for one extra

  return;
}

#define UNUSED_RING_NUMBER -1
#define NEVER_USE_THIS_RING_AGAIN -2

int
Ring_Number_Manager::ok () const
{
  if (_nr < 0)
    return 0;

  if (_nr > 10000)
    cerr << "Ring_Number_Manager::ok:improbably large nrings value " << _nr << endl;

  if (NULL != _ring_id)
  {
    for (int i = 1; i <= _nr; i++)
    {
      if (UNUSED_RING_NUMBER == _ring_id[i])
        continue;

      if (NEVER_USE_THIS_RING_AGAIN == _ring_id[i])
        continue;

      if (_ring_id[i] < 0)
      {
        cerr << "Ring_Number_Manager::ok:ring id " << i << " is " << _ring_id[i] << endl;
        return 0;
      }

      if (_ring_id[i] > 100000)
        cerr << "Ring_Number_Manager::ok:improbably large ring number " << _ring_id[i] << endl;
    }
  }

  if (NULL != _from_atom)
  {
    for (int i = 1; i <= _nr; i++)
    {
      if (_ring_id[i] < 0)
        continue;

      if (_from_atom[i] < 0)
      {
        cerr << "Ring_Number_Manager::ok:from atom " << i << " is " << _from_atom[i] << endl;
        return 0;
      }
    }
  }

  if (NULL != _bond)
  {
    for (int i = 1; i < _nr; i++)
    {
      if (_ring_id[i] < 0)
        continue;

      if (! _bond[i]->ok ())
      {
        cerr << "Ring_Number_Manager::ok:bond " << i << " is bad\n";
        return 0;
      }
    }
  }

  return 1;
}

int
Ring_Number_Manager::activate (int nrings)
{
  assert (nrings > 0);
  assert (NULL == _ring_id);

  _nr = nrings;
  _ring_id = new_int (_nr + 1, UNUSED_RING_NUMBER);
  _bond    = new const Bond * [_nr + 1];
  _from_atom = new atom_number_t[_nr + 1];

  _ring_id[0] = -999;    // ring 0 is never used for smiles

  return 1;
}

Ring_Number_Manager::~Ring_Number_Manager ()
{
  if (NULL != _ring_id)
  {
    delete [] _ring_id;
    delete [] _bond;
    delete []_from_atom;
  }
}

int
Ring_Number_Manager::debug_print (ostream & os) const
{
  os << "Info on Ring_Number_Manager for " << _nr << " rings\n";

  for (int i = 1; i <= _nr; i++)
  {
    if (UNUSED_RING_NUMBER == _ring_id[i])
    {
      os << "ring " << i << " not active\n";
      continue;
    }

    if (NEVER_USE_THIS_RING_AGAIN == _ring_id[i])
    {
      os << "ring " << i << " never to be re-used\n";
      continue;
    }

    os << "Ring " << i << " atom " << _ring_id[i] << " bt = " <<
          _bond[i]->btype () << " other = " << _from_atom[i] << endl;
  }

  return os.good ();
}

/*static void
append_bond_type (IWString & smiles, const Bond * b)
{
  if (b->is_single_bond ())
    ;
  else if (b->is_double_bond ())
    smiles += DOUBLE_BOND_SYMBOL;
  else if (b->is_triple_bond ())
    smiles += TRIPLE_BOND_SYMBOL;
  else if (b->is_aromatic ())
    smiles += AROMATIC_BOND_SYMBOL;
  else if (IS_NOT_A_BOND (b->btype ()))
    smiles += '.';

//  all other bond types are silently ignored

  return;
}*/

/*
  Jul 2003. John Lampe had a case where we may not want to use low ring numbers
  Allow an offset
*/

static int ring_number_offset = 0;

void
set_smiles_ring_number_offset (int s)
{
  ring_number_offset = s;

  return;
}

static char single_digit[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

//#define DEBUG_APPEND_RING_CLOSURE_DIGITS

void
Ring_Number_Manager::_append_ring_closure_digits (IWString & smiles,
                            int ring_closure_number,
                            const Bond * b,
                            atom_number_t ato) const
{
#ifdef DEBUG_APPEND_RING_CLOSURE_DIGITS
  if (NULL == b)
    cerr << "No ring closure bond\n";
  else if (b->is_aromatic ())
    cerr << "closed with an aromatic bond\n";
  else if (b->is_single_bond ())
    cerr << "closed with a single bond\n";
  else if (! b->is_single_bond ())
    cerr << "Non single bond found\n";
#endif

  ring_closure_number += ring_number_offset;

/*if (NULL == b)
    ;
  else if (b->is_aromatic () && include_aromaticity_in_smiles)
    ;
  else if (b->is_permanent_aromatic ())   // never write these
    ;
  else if (! b->is_single_bond ())
    append_bond_type (smiles, b);*/

// Oct 2009. Do not include directional bonds when closing rings - corina hates it
//  ............................... OK
//  ............................... Corina complains incomplete specification and ignores
//  ............................... Corina complains inconsistent and fails

  if (NULL == b)   // is a ring opening
    ;
  else if (! b->is_single_bond())
    b->append_bond_type (smiles, ato, _include_aromaticity_in_smiles);
  else if (b->is_aromatic())
    b->append_bond_type (smiles, ato, _include_aromaticity_in_smiles);
  else if (! b->is_directional())
    b->append_bond_type (smiles, ato, _include_aromaticity_in_smiles);
  else if (! _include_cis_trans_in_smiles)
    b->append_bond_type (smiles, ato, _include_aromaticity_in_smiles);
  else if (FileScope_include_directionality_in_ring_closure_bonds)
    b->append_bond_type (smiles, ato, _include_aromaticity_in_smiles);
  else    // is directional, but don't want it to be marked directional
  {
    set_include_cis_trans_in_smiles(0);
    b->append_bond_type (smiles, ato, _include_aromaticity_in_smiles);
    set_include_cis_trans_in_smiles(1);
  }

  if (ring_closure_number < 10)
    smiles += single_digit[ring_closure_number];
  else
  {
    smiles += '%';
    smiles.append_number (ring_closure_number);
  }

  return;
}

//#define DEBUG_STORE_RING

/*
  A ring opening digit is being created from atom AFROM to atom ATO

*/

int
Ring_Number_Manager::store_ring (IWString & smiles,
                          const Bond * b,
                          atom_number_t ato)
{
  atom_number_t afrom = b->other (ato);

#ifdef DEBUG_STORE_RING
  cerr << "Storing ring opening from atom " << afrom << " to " << ato << ' ';
  if (b->is_single_bond ())
    cerr << "single bond\n";
  else
    cerr << "multiple bond\n";
  for (int i = 0; i <= _nr; i++)
  {
    cerr << "i = " << i << " ring id = " << _ring_id[i] << endl;
  }
#endif

  int free_ring = locate_item_in_array (UNUSED_RING_NUMBER, _nr + 1, _ring_id);
  assert (free_ring > 0);

  _ring_id[free_ring]   = afrom;
  _from_atom[free_ring] = ato;
  _bond[free_ring] = b;

  _append_ring_closure_digits (smiles, free_ring, NULL, ato);   // these are actually ring openings, so no bond

  return 1;
}

/*
  Handle the Concord required ring opening/closing convention as appropriate
*/

int
Ring_Number_Manager::_process_ring (IWString & smiles,
                                    int ring_number,
                                    atom_number_t ato)
{
  assert (ring_number >= 0 && ring_number < _nr);

  _append_ring_closure_digits (smiles, ring_number, _bond[ring_number], ato);

  if (smiles_reuse_ring_closure_numbers ())
    _ring_id[ring_number] = UNUSED_RING_NUMBER;
  else
    _ring_id[ring_number] = NEVER_USE_THIS_RING_AGAIN;

  return 1;
}

int
Ring_Number_Manager::_place_ring_closure (IWString & smiles,
                   atom_number_t a,
                   atom_number_t afrom)
{
  for (int i = 0; i < _nr; i++)
  {
    if (a == _ring_id[i] && afrom == _from_atom[i])
    {
      _process_ring (smiles, i, a);
      return 1;
    }
  }

  cerr << "Ring_Number_Manager::_place_ring_closure: no ring closure from " << 
          a << " to " << afrom << endl;
  debug_print (cerr);
  iwabort ();

  return 0;
}

/*
  Atom A is a chiral centre, and some ring closures end on it.
  We must place the ring closure numbers (bonds) in the correct chiral
  order. The calling routine must have placed RING_CLOSURES_FOUND in
  the correct order for the smiles
*/

int
Ring_Number_Manager::_append_ring_closures_for_chiral_atom (IWString & smiles,
                           atom_number_t a,
                           const resizable_array<atom_number_t> & ring_closures_found)
{
//if (2 != ring_closures_found.number_elements ())
//{
//  cerr << "Ring_Number_Manager::_append_ring_closures_for_chiral_atom: problem at atom " << a << endl;
//  cerr << "Found " << ring_closures_found.number_elements () << " ring closures:";
//  for (int i = 0; i < ring_closures_found.number_elements (); i++)
//    cerr << ' ' << ring_closures_found[i];
//  cerr << endl;
//}

  int nr = ring_closures_found.number_elements ();
  for (int i = 0; i < nr; i++)
  {
    _place_ring_closure (smiles, a, ring_closures_found[i]);
  }

  return 1;
}

static int
locate_item_in_array (int needle, int size_of_haystack, const int * haystack, int istart)
{
  for (int i = istart; i < size_of_haystack; i++)
  {
    if (needle == haystack[i])
      return i;
  }
  
  return -1;
};

//#define DEBUG_APPEND_RING_CLOSURES_FOR_ATOM

/*
  When appending ring closure digits, we must take special care if the
  atom is chiral
*/

int
Ring_Number_Manager::append_ring_closures_for_atom (IWString & smiles,
                   atom_number_t a,
                   const resizable_array<atom_number_t> & ring_closures_found,
                   const Chiral_Centre * c)
{
  assert (ok ());

#ifdef DEBUG_APPEND_RING_CLOSURES_FOR_ATOM
  cerr << "Adding ring closure digits for atom " << a << endl;
  for (int i = 0; i < _nr; i++)
  {
    cerr << "_ring_id[" << i << "] = " << _ring_id[i] << endl;
  }
#endif

  if (NULL != c && ring_closures_found.number_elements () > 1)
    return _append_ring_closures_for_chiral_atom (smiles, a, ring_closures_found);

  int rc = 0;
  int i = 0;
  while ((i = locate_item_in_array (a, _nr + 1, _ring_id, i)) > 0)
  {
    rc += _process_ring (smiles, i, a);    // ato
  }

//for (int i = 0; i < _nr + 1; i++)
//{
//  if (a == _ring_id[i])
//    rc += _process_ring (smiles, i);
//}

  return rc;
}
