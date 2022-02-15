
SOURCES += main.cpp
SOURCES += LayerBitmapAffineBlit.cpp
SOURCES += LayerBitmapUtility.cpp
SOURCES += tvpgl.cpp
PROJECT_BASENAME = layerExAffine

RC_LEGALCOPYRIGHT ?= Copyright (C) 2021-2021 Julian Uy; See details of license at license.txt, or the source code location.

include external/ncbind/Rules.lib.make
