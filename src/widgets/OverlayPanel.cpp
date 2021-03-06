//
//  OverlayPanel.cpp
//  Audacity
//
//  Created by Paul Licameli on 5/7/16.
//
//

#include "../Audacity.h"
#include "OverlayPanel.h"
#include "Overlay.h"
#include "../AColor.h"
#include <algorithm>
#include <wx/dcclient.h>

OverlayPanel::OverlayPanel(wxWindow * parent, wxWindowID id,
             const wxPoint & pos,
             const wxSize & size,
             long style)
: BackedPanel(parent, id, pos, size, style)
{}

void OverlayPanel::AddOverlay(Overlay *pOverlay)
{
   mOverlays.push_back(pOverlay);
}

bool OverlayPanel::RemoveOverlay(Overlay *pOverlay)
{
   const size_t oldSize = mOverlays.size();
   std::remove(mOverlays.begin(), mOverlays.end(), pOverlay);
   return oldSize != mOverlays.size();
}

void OverlayPanel::ClearOverlays()
{
   mOverlays.clear();
}

void OverlayPanel::DrawOverlays(bool repaint, wxDC *pDC)
{
   size_t n_pairs = mOverlays.size();

   std::vector< std::pair<wxRect, bool> > pairs;
   pairs.reserve(n_pairs);

   // Find out the rectangles and outdatedness for each overlay
   wxSize size(GetBackingDC().GetSize());
   for (const auto pOverlay : mOverlays)
      pairs.push_back(pOverlay->GetRectangle(size));

   // See what requires redrawing.  If repainting, all.
   // If not, then whatever is outdated, and whatever will be damaged by
   // undrawing.
   // By redrawing only what needs it, we avoid flashing things like
   // the cursor that are drawn with invert.
   if (!repaint) {
      bool done;
      do {
         done = true;
         for (size_t ii = 0; ii < n_pairs; ++ii) {
            for (size_t jj = ii + 1; jj < n_pairs; ++jj) {
               if (pairs[ii].second != pairs[jj].second &&
                   pairs[ii].first.Intersects(pairs[jj].first)) {
                  done = false;
                  pairs[ii].second = pairs[jj].second = true;
               }
            }
         }
      } while (!done);
   }

   Maybe<wxClientDC> myDC;
   auto &dc = pDC ? *pDC : (myDC.create(this), *myDC);

   // Erase
   bool done = true;
   auto it2 = pairs.begin();
   for (auto pOverlay : mOverlays) {
      if (repaint || it2->second) {
         done = false;
         pOverlay->Erase(dc, GetBackingDC());
      }
      ++it2;
   }

   // Draw
   if (!done) {
      it2 = pairs.begin();
      for (auto pOverlay : mOverlays) {
         if (repaint || it2->second) {
            // Guarantee a clean state of the dc each pass:
            ADCChanger changer{ &dc };

            pOverlay->Draw(*this, dc);
         }
         ++it2;
      }
   }
}

BEGIN_EVENT_TABLE(OverlayPanel, BackedPanel)
END_EVENT_TABLE()
