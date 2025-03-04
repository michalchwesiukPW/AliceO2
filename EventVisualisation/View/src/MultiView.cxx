// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

///
/// \file    MultiView.cxx
/// \author  Jeremi Niedziela

#include "EventVisualisationView/MultiView.h"

#include "EventVisualisationBase/ConfigurationManager.h"
#include "EventVisualisationView/EventManager.h"
#include "EventVisualisationBase/GeometryManager.h"
#include "EventVisualisationDataConverter/VisualisationConstants.h"

#include <TBrowser.h>
#include <TEnv.h>
#include <TEveBrowser.h>
#include <TEveManager.h>
#include <TEveProjectionAxes.h>
#include <TEveProjectionManager.h>
#include <TEveWindowManager.h>

#include "FairLogger.h"

using namespace std;

namespace o2
{
namespace event_visualisation
{

MultiView* MultiView::sInstance = nullptr;

MultiView::MultiView()
{
  // set scene names and descriptions
  mSceneNames[Scene3dGeom] = "3D Geometry Scene";
  mSceneNames[SceneRphiGeom] = "R-Phi Geometry Scene";
  mSceneNames[SceneZrhoGeom] = "Rho-Z Geometry Scene";
  mSceneNames[Scene3dEvent] = "3D Event Scene";
  mSceneNames[SceneRphiEvent] = "R-Phi Event Scene";
  mSceneNames[SceneZrhoEvent] = "Rho-Z Event Scene";

  mSceneDescriptions[Scene3dGeom] = "Scene holding 3D geometry.";
  mSceneDescriptions[SceneRphiGeom] = "Scene holding projected geometry for the R-Phi view.";
  mSceneDescriptions[SceneZrhoGeom] = "Scene holding projected geometry for the Rho-Z view.";
  mSceneDescriptions[Scene3dEvent] = "Scene holding 3D event.";
  mSceneDescriptions[SceneRphiEvent] = "Scene holding projected event for the R-Phi view.";
  mSceneDescriptions[SceneZrhoEvent] = "Scene holding projected event for the Rho-Z view.";

  // spawn scenes
  mScenes[Scene3dGeom] = gEve->GetGlobalScene();
  mScenes[Scene3dGeom]->SetNameTitle(mSceneNames[Scene3dGeom].c_str(), mSceneDescriptions[Scene3dGeom].c_str());

  mScenes[Scene3dEvent] = gEve->GetEventScene();
  mScenes[Scene3dEvent]->SetNameTitle(mSceneNames[Scene3dEvent].c_str(), mSceneDescriptions[Scene3dEvent].c_str());

  for (int i = SceneRphiGeom; i < NumberOfScenes; ++i) {
    mScenes[i] = gEve->SpawnNewScene(mSceneNames[i].c_str(), mSceneDescriptions[i].c_str());
  }

  // remove window manager from the list
  gEve->GetWindowManager()->RemoveFromListTree(gEve->GetListTree(), nullptr);

  // Projection managers
  mProjections[ProjectionRphi] = new TEveProjectionManager();
  mProjections[ProjectionZrho] = new TEveProjectionManager();

  mProjections[ProjectionRphi]->SetProjection(TEveProjection::kPT_RPhi);
  mProjections[ProjectionZrho]->SetProjection(TEveProjection::kPT_RhoZ);

  // open scenes
  gEve->GetScenes()->FindListTreeItem(gEve->GetListTree())->SetOpen(true);

  // add axes
  TEnv settings;
  ConfigurationManager::getInstance().getConfig(settings);
  const bool showAxes = settings.GetValue("axes.show", false);

  if (showAxes) {
    for (int i = 0; i < NumberOfProjections; ++i) {
      TEveProjectionAxes axes(mProjections[static_cast<EProjections>(i)]);
      axes.SetMainColor(kWhite);
      axes.SetTitle("R-Phi");
      axes.SetTitleSize(0.05);
      axes.SetTitleFont(102);
      axes.SetLabelSize(0.025);
      axes.SetLabelFont(102);
      mScenes[getSceneOfProjection(static_cast<EProjections>(i))]->AddElement(&axes);
    }
  }
  setupMultiview();
  sInstance = this;
}

MultiView::~MultiView()
{
  destroyAllGeometries();
}

MultiView* MultiView::getInstance()
{
  if (!sInstance) {
    new MultiView();
  }
  return sInstance;
}

void MultiView::setupMultiview()
{
  // Split window in packs for 3D and projections, create viewers and add scenes to them
  TEveWindowSlot* slot = TEveWindow::CreateWindowInTab(gEve->GetBrowser()->GetTabRight());
  TEveWindowPack* pack = slot->MakePack();

  pack->SetElementName("Multi View");
  pack->SetHorizontal();
  pack->SetShowTitleBar(kFALSE);
  pack->NewSlotWithWeight(2)->MakeCurrent(); // new slot is created from pack

  mViews[View3d] = gEve->SpawnNewViewer("3D View", "");
  mViews[View3d]->AddScene(mScenes[Scene3dGeom]);
  mViews[View3d]->AddScene(mScenes[Scene3dEvent]);

  pack = pack->NewSlot()->MakePack();
  pack->SetNameTitle("2D Views", "");
  pack->SetShowTitleBar(kFALSE);
  pack->NewSlot()->MakeCurrent();
  mViews[ViewRphi] = gEve->SpawnNewViewer("R-Phi View", "");
  mViews[ViewRphi]->GetGLViewer()->SetCurrentCamera(TGLViewer::kCameraOrthoXOY);
  mViews[ViewRphi]->AddScene(mScenes[SceneRphiGeom]);
  mViews[ViewRphi]->AddScene(mScenes[SceneRphiEvent]);

  pack->NewSlot()->MakeCurrent();
  mViews[ViewZrho] = gEve->SpawnNewViewer("Rho-Z View", "");
  mViews[ViewZrho]->GetGLViewer()->SetCurrentCamera(TGLViewer::kCameraOrthoXOY);
  mViews[ViewZrho]->AddScene(mScenes[SceneZrhoGeom]);
  mViews[ViewZrho]->AddScene(mScenes[SceneZrhoEvent]);
}

MultiView::EScenes MultiView::getSceneOfProjection(EProjections projection)
{
  if (projection == ProjectionRphi) {
    return SceneRphiGeom;
  } else if (projection == ProjectionZrho) {
    return SceneZrhoGeom;
  }
  return NumberOfScenes;
}

TEveGeoShape* MultiView::getDetectorGeometry(const std::string& detectorName)
{
  for (const auto& geom : mDetectors) {
    if (geom->GetElementName() == detectorName) {
      return geom;
    }
  }

  return nullptr;
}

void MultiView::drawGeometryForDetector(string detectorName, bool threeD, bool rPhi, bool zRho)
{
  auto& geometryManager = GeometryManager::getInstance();
  TEveGeoShape* shape = geometryManager.getGeometryForDetector(detectorName);
  registerGeometry(shape, threeD, rPhi, zRho);
  mDetectors.push_back(shape);
}

void MultiView::registerGeometry(TEveGeoShape* geom, bool threeD, bool rPhi, bool zRho)
{
  if (!geom) {
    LOG(error) << "MultiView::registerGeometry -- geometry is NULL!";
    exit(-1);
  }
  // mGeomVector.push_back(geom);

  TEveProjectionManager* projection;

  if (threeD) {
    gEve->AddElement(geom, getScene(Scene3dGeom));
  }
  if (rPhi) {
    projection = getProjection(ProjectionRphi);
    projection->SetCurrentDepth(-10);
    projection->ImportElements(geom, getScene(SceneRphiGeom));
    projection->SetCurrentDepth(0);
  }
  if (zRho) {
    projection = getProjection(ProjectionZrho);
    projection->SetCurrentDepth(-10);
    projection->ImportElements(geom, getScene(SceneZrhoGeom));
    projection->SetCurrentDepth(0);
  }
}

void MultiView::destroyAllGeometries()
{
  //  for (unsigned int i = 0; i < mGeomVector.size(); ++i) {
  //    if (mGeomVector[i]) {
  //      mGeomVector[i]->DestroyElements();
  //      gEve->RemoveElement(mGeomVector[i], getScene(Scene3dGeom));
  //      mGeomVector[i] = nullptr;
  //    }
  //  }
  getScene(Scene3dGeom)->DestroyElements();
  getScene(SceneRphiGeom)->DestroyElements();
  getScene(SceneZrhoGeom)->DestroyElements();
  mDetectors.clear();
}

void MultiView::registerElement(TEveElement* event)
{
  gEve->GetCurrentEvent()->AddElement(event);
  getProjection(ProjectionRphi)->ImportElements(event, getScene(SceneRphiEvent));
  getProjection(ProjectionZrho)->ImportElements(event, getScene(SceneZrhoEvent));

  gEve->Redraw3D();
}

void MultiView::destroyAllEvents()
{
  if (gEve->GetCurrentEvent()) {
    gEve->GetCurrentEvent()->RemoveElements();
  }
  getScene(SceneRphiEvent)->DestroyElements();
  getScene(SceneZrhoEvent)->DestroyElements();
}

void MultiView::redraw3D()
{
  gEve->Redraw3D();
}

} // namespace event_visualisation
} // namespace o2
