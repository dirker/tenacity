/**********************************************************************

 Audacity: A Digital Audio Editor

 TrackPanel.h

 Paul Licameli

 **********************************************************************/

#ifndef __AUDACITY_CELLULAR_PANEL__
#define __AUDACITY_CELLULAR_PANEL__

#include <wx/cursor.h>
#include "widgets/OverlayPanel.h"

class ViewInfo;
class AudacityProject;

class TrackPanelCell;
class TrackPanelGroup;
class TrackPanelNode;
struct TrackPanelMouseEvent;
struct TrackPanelMouseState;

class UIHandle;
using UIHandlePtr = std::shared_ptr<UIHandle>;

// This class manages a panel divided into a number of sub-rectangles called
// cells, that each implement hit tests returning click-drag-release handler
// objects, and other services.
// It has no dependency on the Track class.
class AUDACITY_DLL_API CellularPanel : public OverlayPanel {
public:
   CellularPanel(wxWindow * parent, wxWindowID id,
                 const wxPoint & pos,
                 const wxSize & size,
                 ViewInfo *viewInfo,
                 // default as for wxPanel:
                 long style = wxTAB_TRAVERSAL | wxNO_BORDER);
   ~CellularPanel() override;
   
   // Overridables:
   
   virtual AudacityProject *GetProject() const = 0;
   
   // Get the root object defining a recursive subdivision of the panel's
   // area into cells
   virtual std::shared_ptr<TrackPanelNode> Root() = 0;

   virtual wxRect FindRect(const TrackPanelCell &cell) = 0;

   // Structure and functions for generalized visitation of the subdivision
   struct Visitor {
      virtual ~Visitor();
      virtual void VisitCell( const wxRect &rect, TrackPanelCell &cell );
      virtual void BeginGroup( const wxRect &rect, TrackPanelGroup &group );
      virtual void EndGroup( const wxRect &rect, TrackPanelGroup &group );
   };

   // Most general visit
   void Visit( Visitor &visitor );

   // Easier visit when you care only about cells
   using SimpleCellVisitor =
      std::function< void( const wxRect &rect, TrackPanelCell &cell ) >;
   void VisitCells( const SimpleCellVisitor &visitor );

   // Easier visits when you want to visit each node once only
   using SimpleNodeVisitor =
      std::function< void( const wxRect &rect, TrackPanelNode &node ) >;
   void VisitPreorder( const SimpleNodeVisitor &visitor );
   void VisitPostorder( const SimpleNodeVisitor &visitor );

   virtual TrackPanelCell *GetFocusedCell() = 0;
   virtual void SetFocusedCell() = 0;
   
   virtual void ProcessUIHandleResult
   (TrackPanelCell *pClickedCell, TrackPanelCell *pLatestCell,
    unsigned refreshResult) = 0;
   
   virtual void UpdateStatusMessage( const wxString & )  = 0;
   
   // Whether this panel keeps focus after a click and drag, or only borrows
   // it.
   virtual bool TakesFocus() const = 0;
   
public:
   // Find cell by coordinate
   struct FoundCell {
      std::shared_ptr< TrackPanelCell > pCell;
      wxRect rect;
   };

   FoundCell FindCell(int mouseX, int mouseY);

   UIHandlePtr Target();
   
   std::shared_ptr<TrackPanelCell> LastCell() const;
   
   bool IsMouseCaptured();
   
   wxCoord MostRecentXCoord() const;
   
   void HandleCursorForPresentMouseState(bool doHit = true);
   
protected:
   bool HasEscape();
   bool CancelDragging( bool escaping );
   void DoContextMenu( TrackPanelCell *pCell = nullptr );
   void ClearTargets();
   
private:
   void Visit(
      const wxRect &rect, const std::shared_ptr<TrackPanelNode> &node,
      Visitor &visitor );

   bool HasRotation();
   bool ChangeTarget(bool forward, bool cycle);
   
   void OnMouseEvent(wxMouseEvent & event);
   void OnCaptureLost(wxMouseCaptureLostEvent & event);
   void OnCaptureKey(wxCommandEvent & event);
   void OnKeyDown(wxKeyEvent & event);
   void OnChar(wxKeyEvent & event);
   void OnKeyUp(wxKeyEvent & event);
   
   void OnSetFocus(wxFocusEvent & event);
   void OnKillFocus(wxFocusEvent & event);
   
   void OnContextMenu(wxContextMenuEvent & event);
   
   void HandleInterruptedDrag();
   void Uncapture( bool escaping, wxMouseState *pState = nullptr );
   bool HandleEscapeKey(bool down);
   void UpdateMouseState(const wxMouseState &state);
   void HandleModifierKey();
   
   void HandleClick( const TrackPanelMouseEvent &tpmEvent );
   void HandleWheelRotation( TrackPanelMouseEvent &tpmEvent );
   
   void HandleMotion( wxMouseState &state, bool doHit = true );
   void HandleMotion
   ( const TrackPanelMouseState &tpmState, bool doHit = true );
   void Leave();
   
   
protected:
   ViewInfo *mViewInfo;

   // To do: make a drawing method and make this private
   wxMouseState mLastMouseState;

private:
   struct State;
   std::unique_ptr<State> mState;
   
   DECLARE_EVENT_TABLE()
};

#endif
