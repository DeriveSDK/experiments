#pragma once

/**
 * @file RiveTvgRenderer.h
 * Basically a copy of the ThorVG renderer found at https://github.com/rive-app/rive-tizen/
 * Minor changes to support some native Derive types.
 */

// ThorVG
#include <thorvg.h>
// Rive
#include "renderer.hpp"
// Other
#include <vector>
#include <stack>

struct TvgPaint {
	uint8_t color[4];
	float thickness = 1.0f;
	tvg::Fill* gradientFill = nullptr;
	tvg::StrokeJoin join = tvg::StrokeJoin::Bevel;
	tvg::StrokeCap  cap = tvg::StrokeCap::Butt;
	rive::RenderPaintStyle style = rive::RenderPaintStyle::fill;
	bool isGradient = false;
};

struct TvgRenderPath : public rive::RenderPath {
	std::unique_ptr<tvg::Shape> tvgShape;

	TvgRenderPath() : tvgShape( tvg::Shape::gen() ) {}

	void buildShape();
	void reset() override;
	void addRenderPath( rive::RenderPath* path, const rive::Mat2D& transform ) override;
	void fillRule( rive::FillRule value ) override;
	void moveTo( float x, float y ) override;
	void lineTo( float x, float y ) override;
	void cubicTo( float ox, float oy, float ix, float iy, float x, float y ) override;
	void close() override;
};

struct TvgGradientStop {
	unsigned int color;
	float stop;
	TvgGradientStop( unsigned int color, float stop ) : color( color ), stop( stop ) {}
};

class TvgGradientBuilder {
public:
	std::vector<TvgGradientStop> stops;
	float sx, sy, ex, ey;
	virtual ~TvgGradientBuilder() {}
	TvgGradientBuilder( float sx, float sy, float ex, float ey ) :
		sx( sx ), sy( sy ), ex( ex ), ey( ey ) {}

	virtual void make( TvgPaint* paint ) = 0;
};

class TvgRadialGradientBuilder : public TvgGradientBuilder {
public:
	TvgRadialGradientBuilder( float sx, float sy, float ex, float ey ) :
		TvgGradientBuilder( sx, sy, ex, ey ) {}
	void make( TvgPaint* paint ) override;
};

class TvgLinearGradientBuilder : public TvgGradientBuilder {
public:
	TvgLinearGradientBuilder( float sx, float sy, float ex, float ey ) :
		TvgGradientBuilder( sx, sy, ex, ey ) {}
	void make( TvgPaint* paint ) override;
};

class TvgRenderPaint : public rive::RenderPaint {
private:
	TvgPaint m_Paint;
	TvgGradientBuilder* m_GradientBuilder = nullptr;

public:
	TvgPaint* paint() { return &m_Paint; }
	void style( rive::RenderPaintStyle style ) override;
	void color( unsigned int value ) override;
	void thickness( float value ) override;
	void join( rive::StrokeJoin value ) override;
	void cap( rive::StrokeCap value ) override;
	void blendMode( rive::BlendMode value ) override;

	void linearGradient( float sx, float sy, float ex, float ey ) override;
	void radialGradient( float sx, float sy, float ex, float ey ) override;
	void addStop( unsigned int color, float stop ) override;
	void completeGradient() override;
};

/**
	* @brief A renderer to render rive objects to ThorVG
	*/
class RiveRenderer : public rive::Renderer {
private:
	tvg::Scene* m_Scene = nullptr;
	tvg::Shape* m_ClipPath = nullptr;
	tvg::Shape* m_BgClipPath = nullptr;
	rive::Mat2D m_Transform;
	std::stack<rive::Mat2D> m_SavedTransforms;
	rive::Mat2D m_DeriveTransform;

public:
	RiveRenderer( tvg::Scene* scene ) : m_Scene(scene) {}
	void save() override;
	void restore() override;
	void transform( const rive::Mat2D& transform ) override;
	void drawPath( rive::RenderPath* path, rive::RenderPaint* paint ) override;
	void clipPath( rive::RenderPath* path ) override;
	void resetClipPath();
};

