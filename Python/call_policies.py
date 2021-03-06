#! /usr/bin/python
# Copyright 2004 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

import os
from environment import settings

from pyplusplus import decl_wrappers
from pyplusplus.decl_wrappers import return_self
from pyplusplus.decl_wrappers import return_internal_reference
from pyplusplus.decl_wrappers import with_custodian_and_ward
from pyplusplus.decl_wrappers import copy_const_reference
from pyplusplus.decl_wrappers import copy_non_const_reference
from pyplusplus.decl_wrappers import manage_new_object
from pyplusplus.decl_wrappers import reference_existing_object
from pyplusplus.decl_wrappers import return_by_value
from pyplusplus.decl_wrappers import return_opaque_pointer
from pyplusplus.decl_wrappers import return_value_policy

db = {
      "::FX::fx2powerprimes" : return_value_policy( return_by_value )
    , "::FX::FX4Splitter::getBottomLeft" :  return_value_policy( reference_existing_object )
    , "::FX::FX4Splitter::getBottomRight" : return_value_policy( reference_existing_object )
    , "::FX::FX4Splitter::getTopLeft" : return_value_policy( reference_existing_object )
    , "::FX::FX4Splitter::getTopRight" : return_value_policy( reference_existing_object )
    , "::FX::FXAccelTable::targetOfAccel" : return_internal_reference()
    , "::FX::FXACL::owner" : return_internal_reference()
    , "::FX::FXACL::Permissions::setAll" : return_self()
    , "::FX::FXACL::Permissions::setAppend" : return_self()
    , "::FX::FXACL::Permissions::setCopyOnWrite" : return_self()
    , "::FX::FXACL::Permissions::setCreateDirs" : return_self()
    , "::FX::FXACL::Permissions::setCreateFiles" : return_self()
    , "::FX::FXACL::Permissions::setDeleteDirs" : return_self()
    , "::FX::FXACL::Permissions::setDeleteFiles" : return_self()
    , "::FX::FXACL::Permissions::setExecute" : return_self()
    , "::FX::FXACL::Permissions::setGenExecute" : return_self()
    , "::FX::FXACL::Permissions::setGenRead" : return_self()
    , "::FX::FXACL::Permissions::setGenWrite" : return_self()
    , "::FX::FXACL::Permissions::setList" : return_self()
    , "::FX::FXACL::Permissions::setReadAttrs" :  return_self()
    , "::FX::FXACL::Permissions::setReadPerms" :  return_self()
    , "::FX::FXACL::Permissions::setRead" : return_self()
    , "::FX::FXACL::Permissions::setTakeOwnership" : return_self()
    , "::FX::FXACL::Permissions::setTraverse" : return_self()
    , "::FX::FXACL::Permissions::setWriteAttrs" : return_self()
    , "::FX::FXACL::Permissions::setWritePerms" : return_self()
    , "::FX::FXACL::Permissions::setWrite" : return_self()
    , "::FX::FXApp::addChore" : return_value_policy( return_opaque_pointer)
    , "::FX::FXApp::addTimeout" : return_value_policy( return_opaque_pointer)
    , "::FX::FXApp::findWindowAt" : return_internal_reference()
    , "::FX::FXApp::findWindowWithId" : return_internal_reference()
    , "::FX::FXApp::getArgv" : return_internal_reference()
    , "::FX::FXApp::getCursorWindow" : return_internal_reference()
    , "::FX::FXApp::getDefaultCursor" : return_internal_reference()
    , "::FX::FXApp::getDefaultVisual" : return_internal_reference()
    , "::FX::FXApp::getDragWindow" :  return_internal_reference()
    , "::FX::FXApp::getEventLoop" : return_internal_reference()
    , "::FX::FXApp::getFocusWindow" : return_internal_reference()
    , "::FX::FXApp::getModalWindow" : return_internal_reference()
    , "::FX::FXApp::getMonoVisual" :  return_internal_reference()
    , "::FX::FXApp::getNormalFont" :  return_internal_reference()
    , "::FX::FXApp::getPopupWindow" : return_internal_reference()
    , "::FX::FXApp::getPrimaryEventLoop" : return_internal_reference()
    , "::FX::FXApp::getRootWindow" :  return_internal_reference()
    , "::FX::FXApp::getWaitCursor" :  return_internal_reference()
    , "::FX::FXApp::instance" : return_value_policy( reference_existing_object )
    , "::FX::FXApp::reg" : return_internal_reference()
    , "::FX::FXApp::removeChore" : return_value_policy( return_opaque_pointer)
    , "::FX::FXApp::removeTimeout" :  return_value_policy( return_opaque_pointer)
    , "::FX::FXBitmapFrame::getBitmap" : return_internal_reference()
    , "::FX::FXBitmap::getData" : return_internal_reference()
    , "::FX::FXBitmapView::getBitmap" : return_internal_reference()
    , "::FX::FXCharset::clear" : return_self()
    , "::FX::FXColorSelector::acceptButton" : return_internal_reference()
    , "::FX::FXColorSelector::cancelButton" : return_internal_reference()
    , "::FX::FXComboBox::getFont" : return_internal_reference()
    , "::FX::FXDataTarget::getTarget" : return_internal_reference()
    , "::FX::FXDC::getApp" : return_value_policy( reference_existing_object )
    , "::FX::FXDC::getFont" : return_internal_reference()
    , "::FX::FXDC::getStippleBitmap" : return_internal_reference()
    , "::FX::FXDC::getTile" : return_internal_reference()
    , "::FX::FXDelegator::getDelegate" : return_internal_reference()
    , "::FX::FXDirBox::getAssociations" : return_internal_reference()
    , "::FX::FXDirBox::getPathnameItem" : return_internal_reference()
    , "::FX::FXDirItem::getAssoc" :  return_value_policy( reference_existing_object )
    , "::FX::FXDirList::createItem" : return_internal_reference()
    , "::FX::FXDirList::getAssociations" : return_internal_reference()
    , "::FX::FXDirList::getPathnameItem" : return_internal_reference()
    , "::FX::FXDirSelector::acceptButton" : return_internal_reference()
    , "::FX::FXDirSelector::cancelButton" : return_internal_reference()
    , "::FX::FXDockBar::findDockAtSide" : return_value_policy( reference_existing_object )
    , "::FX::FXDockBar::findDockNear" : return_value_policy( reference_existing_object )
    , "::FX::FXDockBar::getDryDock" : return_value_policy( reference_existing_object )
    , "::FX::FXDockBar::getWetDock" : return_value_policy( reference_existing_object )
    , "::FX::FXDockTitle::getFont" : return_internal_reference()
    , "::FX::FXDrawable::getVisual" : return_value_policy( reference_existing_object )
    , "::FX::FXDriveBox::getAssociations" : return_internal_reference()
    , "::FX::FXEventLoop::findWindowAt" : return_internal_reference()
    , "::FX::FXEventLoop::findWindowWithId" : return_internal_reference()
    , "::FX::FXEventLoop::getApp" : return_internal_reference()
    , "::FX::FXEventLoop::getCursorWindow" : return_internal_reference()
    , "::FX::FXEventLoop::getDragWindow" :  return_internal_reference()
    , "::FX::FXEventLoop::getFocusWindow" : return_internal_reference()
    , "::FX::FXEventLoop::getModalWindow" : return_internal_reference()
    , "::FX::FXEventLoop::getPopupWindow" : return_internal_reference()
    , "::FX::FXEventLoop::getRootWindow" :  return_internal_reference()
    , "::FX::FXException::nested" : return_internal_reference()
    , "::FX::FXFileDialog::getFilenames" : return_internal_reference()
    , "::FX::FXFileDialog::getOpenFilenames" : return_internal_reference()
    , "::FX::FXFileDict::associate" : return_internal_reference()
    , "::FX::FXFileDict::findDirBinding" : return_internal_reference()
    , "::FX::FXFileDict::findExecBinding" : return_internal_reference()
    , "::FX::FXFileDict::findFileBinding" : return_internal_reference()
    , "::FX::FXFileDict::find" : return_internal_reference()
    , "::FX::FXFileDict::getIconDict" : return_internal_reference()
    , "::FX::FXFileDict::remove" : return_internal_reference()
    , "::FX::FXFileDict::replace" : return_internal_reference()
    , "::FX::FXFileItem::getAssoc" : return_value_policy( return_opaque_pointer)
    , "::FX::FXFileList::createItem" : return_internal_reference()
    , "::FX::FXFileList::getAssociations" : return_internal_reference()
    , "::FX::FXFileList::getItemAssoc" : return_internal_reference()
    , "::FX::FXFile::name" : return_value_policy( return_by_value)
    , "::FX::FXFileSelector::acceptButton" : return_internal_reference()
    , "::FX::FXFileSelector::cancelButton" : return_internal_reference()
    , "::FX::FXFileSelector::getFilenames" : return_internal_reference()
    , "::FX::FXFileSelector::getSelectedFiles" : return_internal_reference()
    , "::FX::FXFileSelector::getSelectedFilesOnly" : return_internal_reference()
    , "::FX::FXFile::stdio" : return_internal_reference()
    , "::FX::FXFoldingItem::getAbove" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getBelow" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getClosedIcon" :  return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getFirst" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getLast" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getNext" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getOpenIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getParent" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingItem::getPrev" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingList::appendItem" : return_internal_reference()
    , "::FX::FXFoldingList::createItem" : return_internal_reference()
    , "::FX::FXFoldingList::findItemByData" : return_internal_reference()
    , "::FX::FXFoldingList::findItem" : return_internal_reference()
    , "::FX::FXFoldingList::getAnchorItem" :  return_internal_reference()
    , "::FX::FXFoldingList::getCurrentItem" : return_internal_reference()
    , "::FX::FXFoldingList::getCursorItem" :  return_internal_reference()
    , "::FX::FXFoldingList::getFirstItem" : return_internal_reference()
    , "::FX::FXFoldingList::getFont" : return_value_policy( reference_existing_object )
    , "::FX::FXFoldingList::getHeaderIcon" : return_internal_reference()
    , "::FX::FXFoldingList::getHeader" : return_internal_reference()
    , "::FX::FXFoldingList::getItemAt" : return_internal_reference()
    , "::FX::FXFoldingList::getItemClosedIcon" : return_internal_reference()
    , "::FX::FXFoldingList::getItemOpenIcon" : return_internal_reference()
    , "::FX::FXFoldingList::getLastItem" : return_internal_reference()
    , "::FX::FXFoldingList::insertItem" : return_internal_reference()
    , "::FX::FXFoldingList::moveItem" : return_internal_reference()
    , "::FX::FXFoldingList::prependItem" : return_internal_reference()
    , "::FX::FXFontSelector::acceptButton" : return_internal_reference()
    , "::FX::FXFontSelector::cancelButton" : return_internal_reference()
    , "::FX::FXFSMonitor::Change::setModified" : return_self()
    , "::FX::FXFSMonitor::Change::setCreated" : return_self()
    , "::FX::FXFSMonitor::Change::setDeleted" : return_self()
    , "::FX::FXFSMonitor::Change::setRenamed" : return_self()
    , "::FX::FXFSMonitor::Change::setAttrib" : return_self()
    , "::FX::FXFSMonitor::Change::setSecurity" : return_self()
    , "::FX::FXGLContext::getVisual" : return_internal_reference()
    , "::FX::FXGLGroup::child" :  return_value_policy( reference_existing_object )
    , "::FX::FXGLGroup::identify" :  return_value_policy( reference_existing_object )
    , "::FX::FXGLGroup::getList" : return_internal_reference()
    , "::FX::FXGLObject::identify" : return_internal_reference()
    , "::FX::FXGLViewer::getScene" : return_internal_reference()
    , "::FX::FXGLViewer::getSelection" : return_internal_reference()
    , "::FX::FXGLViewer::pick" : return_internal_reference()
    , "::FX::FXGLViewer::processHits" : return_internal_reference()
    , "::FX::FXGroupBox::getFont" : return_internal_reference()
    , "::FX::FXHandedInterfaceI::buttonWell" : return_internal_reference()
    , "::FX::FXHandedInterfaceI::cancelButton" : return_internal_reference()
    , "::FX::FXHandedInterfaceI::okButton" : return_internal_reference()
    , "::FX::FXHandedMsgBox::errorIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXHandedMsgBox::fatalErrorIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXHandedMsgBox::informationalIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXHandedMsgBox::questionIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXHeader::createItem" : return_internal_reference()
    , "::FX::FXHeader::getFont" : return_value_policy( reference_existing_object )
    , "::FX::FXHeader::getItemIcon" : return_internal_reference()
    , "::FX::FXHeader::getItem" : return_internal_reference()
    , "::FX::FXHeaderItem::getIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXIconItem::getBigIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXIconItem::getMiniIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXIconList::createItem" : return_internal_reference()
    , "::FX::FXIconList::getFont" : return_value_policy( reference_existing_object )
    , "::FX::FXIconList::getHeaderIcon" : return_internal_reference()
    , "::FX::FXIconList::getHeader" : return_internal_reference()
    , "::FX::FXIconList::getItemBigIcon" : return_internal_reference()
    , "::FX::FXIconList::getItemMiniIcon" : return_internal_reference()
    , "::FX::FXIconList::getItem" : return_internal_reference()
    , "::FX::FXIconSource::loadIcon" : return_value_policy( manage_new_object )
    , "::FX::FXIconSource::loadImage" : return_value_policy( manage_new_object )
    , "::FX::FXIconSource::loadScaledIcon" : return_value_policy( manage_new_object )
    , "::FX::FXIconSource::loadScaledImage" : return_value_policy( manage_new_object )
    , "::FX::FXId::getApp" : return_value_policy( reference_existing_object )
    , "::FX::FXId::getEventLoop" : return_value_policy( reference_existing_object )
    , "::FX::FXId::getVisual" : return_value_policy( reference_existing_object )
    , "::FX::FXImageFrame::getImage" : return_internal_reference()
    , "::FX::FXImage::getData" : return_internal_reference()
    , "::FX::FXImageView::getImage" : return_internal_reference()
    , "::FX::FXIPCChannel::device" : return_value_policy( reference_existing_object )
    , "::FX::FXIPCChannelIndirector::channel" : return_value_policy( reference_existing_object )
    , "::FX::FXIPCChannel::registry" : return_value_policy( reference_existing_object )
    , "::FX::FXIPCChannel::threadPool" : return_value_policy( reference_existing_object )
    , "::FX::FXLabel::getFont" : return_internal_reference()
    , "::FX::FXLabel::getIcon" : return_internal_reference()
    , "::FX::FXListBox::getFont" : return_internal_reference()
    , "::FX::FXListBox::getItemIcon" : return_internal_reference()
    , "::FX::FXList::createItem" : return_internal_reference()
    , "::FX::FXList::getFont" : return_value_policy( reference_existing_object )
    , "::FX::FXList::getItemIcon" : return_internal_reference()
    , "::FX::FXList::getItem" : return_internal_reference()
    , "::FX::FXListItem::getIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXMat3d::eye" : return_self()
    , "::FX::FXMat3d::rot" : return_self()
    , "::FX::FXMat3d::scale" : return_self()
    , "::FX::FXMat3d::trans" : return_self()
    , "::FX::FXMat3f::eye" : return_self()
    , "::FX::FXMat3f::rot" : return_self()
    , "::FX::FXMat3f::scale" : return_self()
    , "::FX::FXMat3f::trans" : return_self()
    , "::FX::FXMat4d::eye" : return_self()
    , "::FX::FXMat4d::frustum" : return_self()
    , "::FX::FXMat4d::left" : return_self()
    , "::FX::FXMat4d::look" : return_self()
    , "::FX::FXMat4d::ortho" : return_self()
    , "::FX::FXMat4d::rot" : return_self()
    , "::FX::FXMat4d::scale" : return_self()
    , "::FX::FXMat4d::trans" : return_self()
    , "::FX::FXMat4d::xrot" : return_self()
    , "::FX::FXMat4d::yrot" : return_self()
    , "::FX::FXMat4d::zrot" : return_self()
    , "::FX::FXMat4f::eye" : return_self()
    , "::FX::FXMat4f::frustum" : return_self()
    , "::FX::FXMat4f::left" : return_self()
    , "::FX::FXMat4f::look" : return_self()
    , "::FX::FXMat4f::ortho" : return_self()
    , "::FX::FXMat4f::rot" : return_self()
    , "::FX::FXMat4f::scale" : return_self()
    , "::FX::FXMat4f::trans" : return_self()
    , "::FX::FXMat4f::xrot" : return_self()
    , "::FX::FXMat4f::yrot" : return_self()
    , "::FX::FXMat4f::zrot" : return_self()
    , "::FX::FXMatrix::childAtRowCol" : return_internal_reference()
    , "::FX::FXMDIChild::contentWindow" : return_internal_reference()
    , "::FX::FXMDIChild::getFont" : return_internal_reference()
    , "::FX::FXMDIChild::getIcon" : return_internal_reference()
    , "::FX::FXMDIChild::getMenu" : return_internal_reference()
    , "::FX::FXMDIChild::getWindowIcon" : return_internal_reference()
    , "::FX::FXMDIChild::getWindowMenu" : return_internal_reference()
    , "::FX::FXMDIClient::getActiveChild" : return_internal_reference()
    , "::FX::FXMenuButton::getMenu" : return_internal_reference()
    , "::FX::FXMenuCaption::getFont" : return_internal_reference()
    , "::FX::FXMenuCaption::getIcon" : return_internal_reference()
    , "::FX::FXMenuCascade::getMenu" : return_internal_reference()
    , "::FX::FXMenuTitle::getMenu" : return_internal_reference()
    , "::FX::FXMetaClass::getBaseClass" : return_value_policy( reference_existing_object )
    , "::FX::FXMetaClass::getMetaClassFromName" : return_value_policy( reference_existing_object )
    , "::FX::FXMetaClass::makeInstance" : return_value_policy( manage_new_object )
    , "::FX::fxnamefromcolor" : return_value_policy( reference_existing_object )
    , "::FX::FXObject::getMetaClass" : return_internal_reference()
    , "::FX::FXObjectList::append" : return_self()
    , "::FX::FXObjectList::assign" : return_self()
    , "::FX::FXObjectList::clear" : return_self()
    , "::FX::FXObjectList::insert" : return_self()
    , "::FX::FXObjectList::list" : return_internal_reference()
    , "::FX::FXObjectList::prepend" : return_self()
    , "::FX::FXObjectList::remove" : return_self()
    , "::FX::FXObjectList::replace" : return_self()
    , "::FX::FXObject::manufacture" : return_value_policy( manage_new_object )
    , "::FX::FXOptionMenu::getCurrent" : return_internal_reference()
    , "::FX::FXOptionMenu::getMenu" :  return_internal_reference()
    , "::FX::FXOptionMenu::getPopup" : return_internal_reference()
    , "::FX::FXPopup::getGrabOwner" : return_internal_reference()
    , "::FX::FXPopup::getNextActive" : return_internal_reference()
    , "::FX::FXPopup::getPrevActive" : return_internal_reference()
    , "::FX::FXProcess::instance" : return_value_policy( reference_existing_object )
    , "::FX::FXProcess::permissions" : return_internal_reference()
    , "::FX::FXProcess::threadPool" : return_value_policy( reference_existing_object )
    , "::FX::FXProgressBar::getFont" : return_internal_reference()
    , "::FX::FXQuatd::adjust" : return_self()
    , "::FX::FXQuatf::adjust" : return_self()
    , "::FX::FXRanged::clipTo" : return_self()
    , "::FX::FXRanged::include" : return_self()
    , "::FX::FXRangef::clipTo" : return_self()
    , "::FX::FXRangef::include" : return_self()
    , "::FX::FXRealSpinner::getFont" : return_internal_reference()
    , "::FX::FXRecentFiles::getTarget" : return_internal_reference()
    , "::FX::FXRectangle::grow" : return_self()
    , "::FX::FXRectangle::move" : return_self()
    , "::FX::FXRectangle::shrink" : return_self()
    , "::FX::FXRegion::offset" : return_self()
    , "::FX::FXRuler::getFont" : return_internal_reference()
    , "::FX::FXScrollArea::horizontalScrollBar" : return_internal_reference()
    , "::FX::FXScrollArea::verticalScrollBar" :  return_internal_reference()
    , "::FX::FXScrollWindow::contentWindow" : return_internal_reference()
    , "::FX::FXSettings::data" : return_internal_reference()
    , "::FX::FXSettings::find" : return_internal_reference()
    , "::FX::FXSettings::insert" : return_internal_reference()
    , "::FX::FXSettings::remove" : return_internal_reference()
    , "::FX::FXSettings::replace" : return_internal_reference()
    , "::FX::FXShutterItem::getButton" : return_value_policy( reference_existing_object )
    , "::FX::FXShutterItem::getContent" : return_value_policy( reference_existing_object )
    , "::FX::FXSphered::include" : return_self()
    , "::FX::FXSpheref::include" : return_self()
    , "::FX::FXSpinner::getFont" : return_internal_reference()
    , "::FX::FXSplashWindow::getFont" : return_internal_reference()
    , "::FX::FXSplashWindow::getIcon" : return_internal_reference()
    , "::FX::FXSplitter::findHSplit" : return_internal_reference()
    , "::FX::FXSplitter::findVSplit" : return_internal_reference()
    , "::FX::FXSSLKey::asymmetricKey" : return_internal_reference()
    , "::FX::FXSSLKey::setAsymmetricKey" : return_self()
    , "::FX::FXStatusBar::getDragCorner" : return_internal_reference()
    , "::FX::FXStatusBar::getStatusLine" : return_internal_reference()
    , "::FX::FXStatusLine::getFont" : return_internal_reference()
    , "::FX::fxstrdup" : return_value_policy( manage_new_object )
    , "::FX::FXStream::addObject" : return_self()
    , "::FX::FXStream::container" : return_value_policy( reference_existing_object )
    , "::FX::FXStream::device" : return_value_policy( reference_existing_object )
    , "::FX::FXStream::loadObject" : return_self()
    , "::FX::FXStream::load" : return_self()
    , "::FX::FXStream::saveObject" : return_self()
    , "::FX::FXStream::save" : return_self()
    , "::FX::FXTable::createItem" : return_internal_reference()
    , "::FX::FXTable::getColumnHeader" : return_internal_reference()
    , "::FX::FXTable::getControlForItem" : return_internal_reference()
    , "::FX::FXTable::getFont" : return_value_policy( reference_existing_object )
    , "::FX::FXTable::getItemIcon" : return_internal_reference()
    , "::FX::FXTable::getItem" : return_internal_reference()
    , "::FX::FXTable::getRowHeader" : return_internal_reference()
    , "::FX::FXTableItem::getControlFor" : return_internal_reference()
    , "::FX::FXTableItem::getIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXTextField::getFont" : return_internal_reference()
    , "::FX::FXText::getFont" : return_internal_reference()
    , "::FX::FXText::getHiliteStyles" : return_internal_reference()
    , "::FX::FXTime::as_tm" : return_value_policy( reference_existing_object )
    , "::FX::FXTime::set_time_t" : return_self()
    , "::FX::FXTime::set_tm" : return_self()
    , "::FX::FXTime::toLocalTime" : return_self()
    , "::FX::FXTime::toUTC" : return_self()
    , "::FX::FXToggleButton::getAltIcon" : return_internal_reference()
    , "::FX::FXToolBar::findDockAtSide" : return_value_policy( reference_existing_object )
    , "::FX::FXToolBar::findDockNear" : return_value_policy( reference_existing_object )
    , "::FX::FXToolBar::getDryDock" : return_value_policy( reference_existing_object )
    , "::FX::FXToolBar::getWetDock" : return_value_policy( reference_existing_object )
    , "::FX::FXToolTip::getFont" : return_internal_reference()
    , "::FX::FXTopWindow::getIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXTopWindow::getMiniIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getAbove" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getBelow" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getClosedIcon" :  return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getFirst" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getLast" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getNext" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getOpenIcon" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getParent" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeItem::getPrev" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeList::appendItem" : return_internal_reference()
    , "::FX::FXTreeListBox::appendItem" : return_internal_reference()
    , "::FX::FXTreeListBox::findItemByData" : return_internal_reference()
    , "::FX::FXTreeListBox::findItem" : return_internal_reference()
    , "::FX::FXTreeListBox::getCurrentItem" : return_internal_reference()
    , "::FX::FXTreeListBox::getFirstItem" : return_internal_reference()
    , "::FX::FXTreeListBox::getFont" : return_internal_reference()
    , "::FX::FXTreeListBox::getItemClosedIcon" : return_internal_reference()
    , "::FX::FXTreeListBox::getItemData" : return_value_policy( return_opaque_pointer ) #Niall?
    , "::FX::FXTreeListBox::getItemOpenIcon" : return_internal_reference()
    , "::FX::FXTreeListBox::getLastItem" : return_internal_reference()
    , "::FX::FXTreeListBox::insertItem" : return_internal_reference()
    , "::FX::FXTreeListBox::moveItem" : return_internal_reference()
    , "::FX::FXTreeListBox::prependItem" : return_internal_reference()
    , "::FX::FXTreeList::createItem" : return_internal_reference()
    , "::FX::FXTreeList::findItemByData" : return_internal_reference()
    , "::FX::FXTreeList::findItem" : return_internal_reference()
    , "::FX::FXTreeList::getAnchorItem" :  return_internal_reference()
    , "::FX::FXTreeList::getCurrentItem" : return_internal_reference()
    , "::FX::FXTreeList::getCursorItem" :  return_internal_reference()
    , "::FX::FXTreeList::getFirstItem" : return_internal_reference()
    , "::FX::FXTreeList::getFont" : return_value_policy( reference_existing_object )
    , "::FX::FXTreeList::getItemAt" : return_internal_reference()
    , "::FX::FXTreeList::getItemClosedIcon" : return_internal_reference()
    , "::FX::FXTreeList::getItemData" : return_value_policy( return_opaque_pointer ) #Niall?
    , "::FX::FXTreeList::getItemOpenIcon" : return_internal_reference()
    , "::FX::FXTreeList::getLastItem" : return_internal_reference()
    , "::FX::FXTreeList::insertItem" : return_internal_reference()
    , "::FX::FXTreeList::moveItem" : return_internal_reference()
    , "::FX::FXTreeList::prependItem" : return_internal_reference()
    , "::FX::FXTriStateButton::getMaybeIcon" : return_internal_reference()
    , "::FX::FXUndoList::current" : return_internal_reference()
    , "::FX::FXWindow::childAtIndex" : return_internal_reference()
    , "::FX::FXWindow::commonAncestor" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::findDefault" : return_internal_reference()
    , "::FX::FXWindow::findInitial" : return_internal_reference()
    , "::FX::FXWindow::getAccelTable" :  return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getChildAt" : return_internal_reference()
    , "::FX::FXWindow::getDefaultCursor" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getDragCursor" :  return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getFirst" : return_internal_reference()
    , "::FX::FXWindow::getFocus" : return_internal_reference()
    , "::FX::FXWindow::getLast" : return_internal_reference()
    , "::FX::FXWindow::getNext" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getOwner" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getParent" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getPrev" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getRoot" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getSavedCursor" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getShell" : return_value_policy( reference_existing_object )
    , "::FX::FXWindow::getTarget" : return_value_policy( reference_existing_object )
    , "::FX::FXWizard::advanceButton" : return_internal_reference()
    , "::FX::FXWizard::buttonFrame" : return_internal_reference()
    , "::FX::FXWizard::cancelButton" : return_internal_reference()
    , "::FX::FXWizard::finishButton" : return_internal_reference()
    , "::FX::FXWizard::getContainer" : return_internal_reference()
    , "::FX::FXWizard::getImage" : return_internal_reference()
    , "::FX::FXWizard::retreatButton" : return_internal_reference()
    , "::FX::QBlkSocket::waitForConnection" : return_value_policy( manage_new_object )
    , "::FX::QBuffer::buffer" : return_internal_reference()
    , "::FX::QBZip2Device::BZ2Data" : return_value_policy( reference_existing_object )
    , "::FX::QDir::entryInfoList" : return_internal_reference()
    , "::FX::QGZipDevice::GZData" : return_value_policy( reference_existing_object )
    , "::FX::QIODevice::permissions" : return_internal_reference()
    , "::FX::QMemArray<unsigned char>::data" : return_internal_reference()
    , "::FX::QMemArray<unsigned char>::operator unsigned char const *" : return_internal_reference()
    , "::FX::QMemArray<unsigned char>::assign" : return_self()
    , "::FX::QMemArray<unsigned char>::duplicate" : return_self()
    , "::FX::QMemArray<unsigned char>::setRawData" : return_self()
    , "::FX::QMemArray<unsigned char>::at" : return_value_policy( return_by_value )
    , "::FX::QMemArray<unsigned char>::begin" : return_internal_reference()
    , "::FX::QMemArray<unsigned char>::end" : return_internal_reference()
    , "::FX::QMemMap::mapIn" : return_value_policy( return_opaque_pointer )
    , "::FX::QPtrVector<FX::FXWindow>::data" : return_internal_reference()
    , "::FX::QPtrVector<FX::FXWindow>::at" : return_internal_reference()
    , "::FX::QPtrVector<FX::FXWindow>::getFirst" : return_internal_reference()
    , "::FX::QPtrVector<FX::FXWindow>::getLast" : return_internal_reference()
    , "::FX::QPtrVector<FX::FXWindow>::first" : return_internal_reference()
    , "::FX::QPtrVector<FX::FXWindow>::last" : return_internal_reference()
    , "::FX::QPtrVector<FX::FXWindow>::int_vector"    : return_self()
    , "::FX::QPtrVector<FX::Generic::BoundFunctorV>::data" : return_internal_reference()
    , "::FX::QPtrVector<FX::Generic::BoundFunctorV>::at" : return_internal_reference()
    , "::FX::QPtrVector<FX::Generic::BoundFunctorV>::getFirst" : return_internal_reference()
    , "::FX::QPtrVector<FX::Generic::BoundFunctorV>::getLast" : return_internal_reference()
    , "::FX::QPtrVector<FX::Generic::BoundFunctorV>::first" : return_internal_reference()
    , "::FX::QPtrVector<FX::Generic::BoundFunctorV>::last" : return_internal_reference()
    , "::FX::QPtrVector<FX::Generic::BoundFunctorV>::int_vector" : return_self()
    , "::FX::QSSLDevice::encryptedDev" : return_internal_reference()
    , "::FX::QThread::creator" : return_value_policy( reference_existing_object )
    , "::FX::QThread::current" : return_value_policy( reference_existing_object )
    , "::FX::QThreadPool::dispatch" : return_internal_reference()
    , "::FX::QThread::primaryThread" :  return_value_policy( reference_existing_object )
    , "::FX::QThread::result" : return_internal_reference()
    , "::FX::QTransString::arg" : return_self()
    #, "::FX::TnFXAppEventLoop::executeRetCode" : return_value_policy( reference_existing_object ) Niall?
    , "::FX::TnFXAppEventLoop::getApp" : return_internal_reference()
    , "::FX::TnFXAppEventLoop::getEventLoop" : return_internal_reference()
    , "::FX::TnFXApp::getEventLoop" : return_internal_reference()
    , "::FX::TnFXApp::instance" : return_value_policy( reference_existing_object )
    , "::FX::TnFXSQLDB::Capabilities::setAsynchronous" : return_self()
    , "::FX::TnFXSQLDB::Capabilities::setHasBackwardsCursor" : return_self()
    , "::FX::TnFXSQLDB::Capabilities::setHasSettableCursor" : return_self()
    , "::FX::TnFXSQLDB::Capabilities::setHasStaticCursor" : return_self()
    , "::FX::TnFXSQLDB::Capabilities::setNoTypeConstraints" : return_self()
    , "::FX::TnFXSQLDB::Capabilities::setQueryRows" : return_self()
    , "::FX::TnFXSQLDB::Capabilities::setTransactions" : return_self()
    , "::FX::TnFXSQLDBColumn::cursor" : return_value_policy( reference_existing_object )
    , "::FX::TnFXSQLDBCursor::statement" : return_value_policy( reference_existing_object )
    , "::FX::TnFXSQLDBRegistry::processRegistry" : return_value_policy( reference_existing_object )
    , "::FX::TnFXSQLDBServer::addDatabase" : return_self()
    , "::FX::TnFXSQLDBStatement::bind" : return_self()
    , "::FX::TnFXSQLDBStatement::driver" : return_value_policy( reference_existing_object )
    , "::FX::TnFXSQLDB_ipc::setIsAsynchronous" : return_self()
    , "::FX::TnFXSQLDB_ipc::setPrefetching" : return_self()
    , "::FX::FXObjectListOf<FX::FXListItem>::list" : return_internal_reference()
    , "::FX::FXObjectListOf<FX::FXGLObject>::list" : return_internal_reference()
    , "::FX::FXObjectListOf<FX::FXHeaderItem>::list" : return_internal_reference()
    , "::FX::FXObjectListOf<FX::FXIconItem>::list" : return_internal_reference()
}
