/** 
 * greebo: Precompiled header module. This is included by the respective precompiled.h
 * files throughout the project. Many of those include boost headers into the
 * pre-compilation, and they do so by #include'ing this file.
 */
#pragma once

// Add DarkRadiant interfaces
#include "Bounded.h"
#include "editable.h"
#include "GLProgramAttributes.h"
#include "i18n.h"
#include "iarchive.h"
#include "ibrush.h"
#include "icamera.h"
#include "iclipper.h"
#include "icommandsystem.h"
#include "icounter.h"
#include "icurve.h"
#include "idatastream.h"
#include "idialogmanager.h"
#include "ieclass.h"
#include "ientity.h"
#include "ientityinspector.h"
#include "ieventmanager.h"
#include "ifilechooser.h"
#include "ifilesystem.h"
#include "ifiletypes.h"
#include "ifilter.h"
#include "ifiltermenu.h"
#include "ifonts.h"
#include "igame.h"
#include "igl.h"
#include "iglprogram.h"
#include "iglrender.h"
#include "igrid.h"
#include "igroupdialog.h"
#include "igroupnode.h"
#include "iimage.h"
#include "ilayer.h"
#include "imainframe.h"
#include "imainframelayout.h"
#include "imap.h"
#include "imapformat.h"
//#include "imapcompiler.h"
#include "imapresource.h"
#include "imd5anim.h"
#include "imd5model.h"
#include "imenu.h"
#include "imodel.h"
#include "imodelcache.h"
#include "imodelsurface.h"
#include "imodule.h"
#include "inameobserver.h"
#include "inamespace.h"
#include "inode.h"
#include "iorthocontextmenu.h"
#include "iorthoview.h"
#include "iparticlenode.h"
#include "iparticles.h"
#include "iparticlestage.h"
#include "ipatch.h"
#include "ipath.h"
#include "ipreferencesystem.h"
#include "iradiant.h"
#include "iregistry.h"
#include "irender.h"
#include "irenderable.h"
#include "irendersystemfactory.h"
#include "iscenegraph.h"
#include "iscenegraphfactory.h"
#include "iscript.h"
#include "iselectable.h"
#include "iselection.h"
#include "iselectionset.h"
#include "iselectiontest.h"
#include "ishaderexpression.h"
#include "ishaders.h"
#include "isound.h"
#include "ispacepartition.h"
#include "itexdef.h"
#include "itextstream.h"
#include "itransformable.h"
#include "itransformnode.h"
#include "iuimanager.h"
#include "iundo.h"
#include "ivolumetest.h"
#include "mapfile.h"
#include "modelskin.h"
#include "ModResource.h"
#include "ShaderLayer.h"
#include "StringSerialisable.h"
#include "Texture.h"
#include "VolumeIntersectionValue.h"
#include "warnings.h"
#include "windowobserver.h"

// Include registry methods
#include "registry/registry.h"
