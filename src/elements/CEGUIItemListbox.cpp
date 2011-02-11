/************************************************************************
    filename:   CEGUIItemListbox.h
    created:    Tue Sep 27 2005
    author:     Tomas Lindquist Olsen
*************************************************************************/
/*************************************************************************
    Crazy Eddie's GUI System (http://www.cegui.org.uk)
    Copyright (C)2004 - 2005 Paul D Turner (paul@cegui.org.uk)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*************************************************************************/
#include "elements/CEGUIItemListbox.h"
#include "CEGUIExceptions.h"

// begin CEGUI namespace
namespace CEGUI
{

/*************************************************************************
    Constants
*************************************************************************/
// event strings
const String ItemListbox::EventNamespace("ItemListbox");
const String ItemListbox::WidgetTypeName("CEGUI/ItemListbox");
const String ItemListbox::EventSelectionChanged("SelectionChanged");
const String ItemListbox::EventMultiSelectModeChanged("MultiSelectModeChanged");

/*************************************************************************
    Properties
*************************************************************************/
ItemListboxProperties::MultiSelect ItemListbox::d_multiSelectProperty;

/************************************************************************
    Constructor
************************************************************************/
ItemListbox::ItemListbox(const String& type, const String& name) :
    ScrolledItemListBase(type, name),
    d_multiSelect(false),
    d_lastSelected(0)
{
    addItemListboxProperties();
}

/************************************************************************
    Layout items
************************************************************************/
void ItemListbox::layoutItemWidgets()
{
    float y = 0;
    float widest = 0;

    ItemEntryList::iterator i = d_listItems.begin();
    ItemEntryList::iterator end = d_listItems.end();
    uint count = 0;
    while (i!=end)
    {
        ItemEntry* entry = *i;

        const Size pxs = entry->getItemPixelSize();
        if (pxs.d_width > widest)
        {
            widest = pxs.d_width;
        }

        entry->setWindowArea(URect(
            UVector2(cegui_absdim(0), cegui_absdim(y)),
            UVector2(cegui_reldim(1), cegui_absdim(y+pxs.d_height))
            ));

        y+=pxs.d_height;

        ++i;
        ++count;
    }

    // reconfigure scrollbars
    configureScrollbars(Size(widest,y));
}

/************************************************************************
    Get size of items
************************************************************************/
Size ItemListbox::getContentSize() const
{
    float h = 0;

    ItemEntryList::const_iterator i = d_listItems.begin();
    ItemEntryList::const_iterator end = d_listItems.end();
    while (i!=end)
    {
        h += (*i)->getItemPixelSize().d_height;
        i++;
    }

    return Size(getItemRenderArea().getWidth(), h);
}

/************************************************************************
    Get the number of selected items
************************************************************************/
size_t ItemListbox::getSelectedCount() const
{
    if (!d_multiSelect)
    {
        return d_lastSelected ? 1 : 0;
    }

    size_t count = 0;
    size_t max = d_listItems.size();
    for (size_t i=0; i<max; ++i)
    {
        const ItemEntry* li = d_listItems[i];
        if (li && li->isSelected())
        {
            ++count;
        }
    }

    return count;
}

/************************************************************************
    Get a pointer to the first selected item
************************************************************************/
ItemEntry* ItemListbox::getFirstSelectedItem() const
{
    if (!d_multiSelect)
    {
        return d_lastSelected;
    }

    size_t max = d_listItems.size();
    for (size_t i=0; i<max; ++i)
    {
        ItemEntry* li = d_listItems[i];
        if (li && li->isSelected())
        {
            return li;
        }
    }

    return 0;
}

/************************************************************************
    Get a pointer to the next selected item
************************************************************************/
ItemEntry* ItemListbox::getNextSelectedItem(const ItemEntry* start_item) const
{
    if (start_item==0||!d_multiSelect)
    {
        return 0;
    }

    size_t max = d_listItems.size();
    size_t i = getItemIndex(start_item);

    while (i<max)
    {
        ItemEntry* li = d_listItems[i];
        if (li && li->isSelected())
        {
            return li;
        }
        ++i;
    }

    return 0;
}

/************************************************************************
    Set whether multiple selections should be allowed
************************************************************************/
void ItemListbox::setMultiSelectEnabled(bool state)
{
    if (state != d_multiSelect)
    {
        d_multiSelect = state;
        WindowEventArgs e(this);
        onMultiSelectModeChanged(e);
    }
}

/************************************************************************
    Notify item clicked
************************************************************************/
void ItemListbox::notifyItemClicked(ItemEntry* li)
{
    bool sel_state = !li->isSelected();
    bool skip = false;

    // multiselect enabled
    if (d_multiSelect)
    {
        uint syskeys = System::getSingletonPtr()->getSystemKeys();
        ItemEntry* last = d_lastSelected;

        // no Control? clear others
        if (!(syskeys & Control))
        {
            clearAllSelections();
            if (!sel_state)
            {
                sel_state=true;
            }
        }

        // select range if Shift if held, and we have a 'last selection'
        if (last && (syskeys & Shift))
        {
            selectRange(getItemIndex(last),getItemIndex(li));
            skip = true;
        }
    }
    else
    {
        clearAllSelections();
    }

    if (!skip)
    {
        li->setSelected_impl(sel_state,false);
        if (sel_state)
        {
            d_lastSelected = li;
        }
        else if (d_lastSelected == li)
        {
            d_lastSelected = 0;
        }
    }

    WindowEventArgs e(this);
    onSelectionChanged(e);
}

/************************************************************************
    Notify item select state change
************************************************************************/
void ItemListbox::notifyItemSelectState(ItemEntry* li, bool state)
{
    // deselect
    if (!state)
    {
        // clear last selection if this one was it
        if (d_lastSelected == li)
        {
            d_lastSelected = 0;
        }
    }
    // if we dont support multiselect, we must clear all the other selections
    else if (!d_multiSelect)
    {
        clearAllSelections();
        li->setSelected_impl(true,false);
        d_lastSelected = li;
    }

    WindowEventArgs e(this);
    onSelectionChanged(e);
}

/*************************************************************************
    Add ItemListbox specific properties
*************************************************************************/
void ItemListbox::addItemListboxProperties()
{
    addProperty(&d_multiSelectProperty);
}

/*************************************************************************
    Query item selection state
*************************************************************************/
bool ItemListbox::isItemSelected(size_t index) const
{
    if (index >= d_listItems.size())
    {
        throw InvalidRequestException("ItemListbox::isItemSelected - The index given is out of range for this ItemListbox");
    }
    ItemEntry *li = d_listItems[index];
    return li ? li->isSelected() : false;
}

/*************************************************************************
    Clear all selections
*************************************************************************/
void ItemListbox::clearAllSelections()
{
    size_t max = d_listItems.size();
    for (size_t i=0; i<max; ++i)
    {
        ItemEntry* item = d_listItems[i];
        if (item)
        {
            item->setSelected_impl(false,false);
        }
    }
    d_lastSelected = 0;

    WindowEventArgs e(this);
    onSelectionChanged(e);
}

/*************************************************************************
    Select range of items
*************************************************************************/
void ItemListbox::selectRange(size_t a, size_t z)
{
    size_t max = d_listItems.size();

    if (a >= max)
    {
        a = 0;
    }
    if (z >= max)
    {
        z = max-1;
    }

    if (a>z)
    {
        size_t tmp;
        tmp = a;
        a = z;
        z = tmp;
    }

    for (size_t i=a; i<=z; ++i)
    {
        ItemEntry* item = d_listItems[i];
        if (item)
        {
            d_lastSelected = item;
            item->setSelected_impl(true,false);
        }
    }

    WindowEventArgs e(this);
    onSelectionChanged(e);
}

/************************************************************************
    Select all items if allowed
************************************************************************/
void ItemListbox::selectAllItems()
{
    if (!d_multiSelect)
    {
        return;
    }

    size_t max = d_listItems.size();
    for (size_t i=0; i<max; ++i)
    {
        ItemEntry* item = d_listItems[i];
        if (item)
        {
            d_lastSelected = item;
            item->setSelected_impl(true,false);
        }
    }

    WindowEventArgs e(this);
    onSelectionChanged(e);
}

/************************************************************************
    Handle selection changed
************************************************************************/
void ItemListbox::onSelectionChanged(WindowEventArgs& e)
{
    fireEvent(EventSelectionChanged, e);
}

/************************************************************************
    Handle multiselect mode changed
************************************************************************/
void ItemListbox::onMultiSelectModeChanged(WindowEventArgs& e)
{
    fireEvent(EventMultiSelectModeChanged, e);
}

/************************************************************************
    Handle key down event
************************************************************************/
void ItemListbox::onKeyDown(KeyEventArgs& e)
{
    ScrolledItemListBase::onKeyDown(e);

    // select all (if allowed) on Ctrl+A
    if (d_multiSelect)
    {
        uint sysKeys = System::getSingletonPtr()->getSystemKeys();
        if (e.scancode == Key::A && (sysKeys&Control))
        {
            selectAllItems();
            e.handled = true;
        }
    }
}

} // end CEGUI namespace