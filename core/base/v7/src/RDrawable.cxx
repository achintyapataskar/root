/// \file RDrawable.cxx
/// \ingroup Base ROOT7
/// \author Axel Naumann <axel@cern.ch>
/// \date 2015-07-08
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2015, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "ROOT/RDrawable.hxx"

#include <cassert>

// pin vtable
ROOT::Experimental::RDrawable::~RDrawable() {}

void ROOT::Experimental::RDrawable::Execute(const std::string &)
{
   assert(false && "Did not expect a menu item to be invoked!");
}


ROOT::Experimental::RDrawableAttributesContainer ROOT::Experimental::RDrawableAttributesNew::fNoDefaults = {};



const char *ROOT::Experimental::RDrawableAttributesNew::Eval(const std::string &name)
{
   if (fDrawable.fNewAttributes) {
      auto entry = fDrawable.fNewAttributes->find(name);
      if (entry != fDrawable.fNewAttributes->end())
         return entry->second.c_str();
   }

   auto entry = fDefaults.find(name);
   if (entry != fDefaults.end())
     return entry->second.c_str();

   return nullptr;
}
