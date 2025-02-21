/***************************************************************************
  qgsrubberband3d.cpp
  --------------------------------------
  Date                 : June 2021
  Copyright            : (C) 2021 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrubberband3d.h"

#include "qgswindow3dengine.h"
#include "qgslinevertexdata_p.h"
#include "qgslinematerial_p.h"
#include "qgsvertexid.h"
#include "qgslinestring.h"

#include <Qt3DCore/QEntity>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometry>
#else
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QGeometry>
#endif

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QMaterial>
#include <QColor>


/// @cond PRIVATE


QgsRubberBand3D::QgsRubberBand3D( Qgs3DMapSettings &map, QgsWindow3DEngine *engine, Qt3DCore::QEntity *parentEntity )
{
  mMapSettings = &map;

  mEntity = new Qt3DCore::QEntity( parentEntity );

  QgsLineVertexData dummyLineData;
  mGeometry = dummyLineData.createGeometry( mEntity );

  Q_ASSERT( mGeometry->attributes().count() == 2 );
  mPositionAttribute = mGeometry->attributes()[0];
  mIndexAttribute = mGeometry->attributes()[1];

  mGeomRenderer = new Qt3DRender::QGeometryRenderer;
  mGeomRenderer->setPrimitiveType( Qt3DRender::QGeometryRenderer::LineStripAdjacency );
  mGeomRenderer->setGeometry( mGeometry );
  mGeomRenderer->setPrimitiveRestartEnabled( true );
  mGeomRenderer->setRestartIndexValue( 0 );

  mEntity->addComponent( mGeomRenderer );

  mLineMaterial = new QgsLineMaterial;
  mLineMaterial->setLineWidth( 3 );
  mLineMaterial->setLineColor( Qt::red );

  QObject::connect( engine, &QgsAbstract3DEngine::sizeChanged, mLineMaterial, [this, engine]
  {
    mLineMaterial->setViewportSize( engine->size() );
  } );
  mLineMaterial->setViewportSize( engine->size() );

  mEntity->addComponent( mLineMaterial );
}

QgsRubberBand3D::~QgsRubberBand3D()
{
  delete mEntity;
}

float QgsRubberBand3D::width() const
{
  return mLineMaterial->lineWidth();
}

void QgsRubberBand3D::setWidth( float width )
{
  mLineMaterial->setLineWidth( width );
}

QColor QgsRubberBand3D::color() const
{
  return mLineMaterial->lineColor();
}

void QgsRubberBand3D::setColor( QColor color )
{
  mLineMaterial->setLineColor( color );
}

void QgsRubberBand3D::reset()
{
  mLineString.clear();
  updateGeometry();
}

void QgsRubberBand3D::addPoint( const QgsPoint &pt )
{
  mLineString.addVertex( pt );
  updateGeometry();
}

void QgsRubberBand3D::removeLastPoint()
{
  const int lastVertexIndex = mLineString.numPoints() - 1;
  mLineString.deleteVertex( QgsVertexId( 0, 0, lastVertexIndex ) );
  updateGeometry();
}

void QgsRubberBand3D::updateGeometry()
{
  QgsLineVertexData lineData;
  lineData.withAdjacency = true;
  lineData.init( Qgis::AltitudeClamping::Absolute, Qgis::AltitudeBinding::Vertex, 0, mMapSettings );
  lineData.addLineString( mLineString );

  mPositionAttribute->buffer()->setData( lineData.createVertexBuffer() );
  mIndexAttribute->buffer()->setData( lineData.createIndexBuffer() );
  mGeomRenderer->setVertexCount( lineData.indexes.count() );
}

/// @endcond
