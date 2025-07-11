// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/geometry/geometry.h"

#include <memory>
#include <optional>

#include "impeller/entity/contents/content_context.h"
#include "impeller/entity/contents/pipelines.h"
#include "impeller/entity/geometry/arc_geometry.h"
#include "impeller/entity/geometry/circle_geometry.h"
#include "impeller/entity/geometry/cover_geometry.h"
#include "impeller/entity/geometry/ellipse_geometry.h"
#include "impeller/entity/geometry/fill_path_geometry.h"
#include "impeller/entity/geometry/line_geometry.h"
#include "impeller/entity/geometry/rect_geometry.h"
#include "impeller/entity/geometry/round_rect_geometry.h"
#include "impeller/entity/geometry/round_superellipse_geometry.h"
#include "impeller/entity/geometry/stroke_path_geometry.h"
#include "impeller/geometry/rect.h"

namespace impeller {

GeometryResult Geometry::ComputePositionGeometry(
    const ContentContext& renderer,
    const Tessellator::VertexGenerator& generator,
    const Entity& entity,
    RenderPass& pass) {
  using VT = SolidFillVertexShader::PerVertexData;

  size_t count = generator.GetVertexCount();

  return GeometryResult{
      .type = generator.GetTriangleType(),
      .vertex_buffer =
          {
              .vertex_buffer = renderer.GetTransientsBuffer().Emplace(
                  count * sizeof(VT), alignof(VT),
                  [&generator](uint8_t* buffer) {
                    auto vertices = reinterpret_cast<VT*>(buffer);
                    generator.GenerateVertices([&vertices](const Point& p) {
                      *vertices++ = {
                          .position = p,
                      };
                    });
                    FML_DCHECK(vertices == reinterpret_cast<VT*>(buffer) +
                                               generator.GetVertexCount());
                  }),
              .vertex_count = count,
              .index_type = IndexType::kNone,
          },
      .transform = entity.GetShaderTransform(pass),
  };
}

GeometryResult::Mode Geometry::GetResultMode() const {
  return GeometryResult::Mode::kNormal;
}

std::unique_ptr<Geometry> Geometry::MakeFillPath(
    const flutter::DlPath& path,
    std::optional<Rect> inner_rect) {
  return std::make_unique<FillPathGeometry>(path, inner_rect);
}

std::unique_ptr<Geometry> Geometry::MakeStrokePath(
    const flutter::DlPath& path,
    const StrokeParameters& stroke) {
  // Skia behaves like this.
  StrokeParameters parameters = stroke;
  if (parameters.miter_limit < 0) {
    parameters.miter_limit = 4.0;
  }
  return std::make_unique<StrokePathGeometry>(path, parameters);
}

std::unique_ptr<Geometry> Geometry::MakeCover() {
  return std::make_unique<CoverGeometry>();
}

std::unique_ptr<Geometry> Geometry::MakeRect(const Rect& rect) {
  return std::make_unique<FillRectGeometry>(rect);
}

std::unique_ptr<Geometry> Geometry::MakeOval(const Rect& rect) {
  return std::make_unique<EllipseGeometry>(rect);
}

std::unique_ptr<Geometry> Geometry::MakeLine(const Point& p0,
                                             const Point& p1,
                                             const StrokeParameters& stroke) {
  return std::make_unique<LineGeometry>(p0, p1, stroke);
}

std::unique_ptr<Geometry> Geometry::MakeCircle(const Point& center,
                                               Scalar radius) {
  return std::make_unique<CircleGeometry>(center, radius);
}

std::unique_ptr<Geometry> Geometry::MakeStrokedCircle(const Point& center,
                                                      Scalar radius,
                                                      Scalar stroke_width) {
  return std::make_unique<CircleGeometry>(center, radius, stroke_width);
}

std::unique_ptr<Geometry> Geometry::MakeFilledArc(const Rect& oval_bounds,
                                                  Degrees start,
                                                  Degrees sweep,
                                                  bool include_center) {
  return std::make_unique<ArcGeometry>(
      Arc(oval_bounds, start, sweep, include_center));
}

std::unique_ptr<Geometry> Geometry::MakeStrokedArc(
    const Rect& oval_bounds,
    Degrees start,
    Degrees sweep,
    const StrokeParameters& stroke) {
  return std::make_unique<ArcGeometry>(Arc(oval_bounds, start, sweep, false),
                                       stroke);
}

std::unique_ptr<Geometry> Geometry::MakeRoundRect(const Rect& rect,
                                                  const Size& radii) {
  return std::make_unique<RoundRectGeometry>(rect, radii);
}

std::unique_ptr<Geometry> Geometry::MakeRoundSuperellipse(
    const Rect& rect,
    Scalar corner_radius) {
  return std::make_unique<RoundSuperellipseGeometry>(rect, corner_radius);
}

bool Geometry::CoversArea(const Matrix& transform, const Rect& rect) const {
  return false;
}

bool Geometry::IsAxisAlignedRect() const {
  return false;
}

bool Geometry::CanApplyMaskFilter() const {
  return true;
}

// static
Scalar Geometry::ComputeStrokeAlphaCoverage(const Matrix& transform,
                                            Scalar stroke_width) {
  Scalar scaled_stroke_width = transform.GetMaxBasisLengthXY() * stroke_width;
  if (scaled_stroke_width == 0.0 || scaled_stroke_width >= kMinStrokeSize) {
    return 1.0;
  }
  // This scalling is eyeballed from Skia.
  return std::clamp(scaled_stroke_width * 2.0f, 0.f, 1.f);
}

}  // namespace impeller
