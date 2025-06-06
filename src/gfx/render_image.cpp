/*
 *  graphtool.cpp
 *  BoE
 *
 *  Created by Celtic Minstrel on 16/04/09.
 *
 */

#include "render_image.hpp"

#include <iostream>
#include <fstream>
#include <boost/filesystem/path.hpp>

#include "fileio/fileio.hpp"
#include "gfx/render_text.hpp"
#include "winutil.hpp"

sf::Shader maskShader;
extern fs::path progDir;

void init_shaders() {
	fs::path shaderPath = progDir/"data"/"shaders";
	fs::path fragPath = shaderPath/"mask.frag", vertPath = shaderPath/"mask.vert";
	
	do {
		std::ifstream fin;
		fin.open(fragPath.string().c_str());
		if(!fin.good()) {
			std::cerr << std_fmterr << ": Error loading fragment shader" << fragPath << std::endl;
			break;
		}
		fin.seekg(0, std::ios::end);
		int size = fin.tellg();
		fin.seekg(0);
		char* fbuf = new char[size + 1];
		fbuf[size] = 0;
		fin.read(fbuf, size);
		fbuf[fin.gcount()] = 0;
		fin.close();

		fin.open(vertPath.string().c_str());
		if(!fin.good()) {
			std::cerr << std_fmterr << ": Error loading vertex shader" << vertPath << std::endl;
			delete[] fbuf;
			break;
		}
		fin.seekg(0, std::ios::end);
		size = fin.tellg();
		fin.seekg(0);
		char* vbuf = new char[size + 1];
		vbuf[size] = 0;
		fin.read(vbuf, size);
		vbuf[fin.gcount()] = 0;
	
		if(!maskShader.loadFromMemory(vbuf, fbuf)) {
			std::cerr << "Error: Failed to load shaders from " << shaderPath << "\nVertex:\n" << vbuf << "\nFragment:\n" << fbuf << std::endl;
		}
		delete[] fbuf;
		delete[] vbuf;
	} while(false);
}

void draw_splash(const sf::Texture& splash, sf::RenderWindow& targ, rectangle dest_rect) {
	rectangle from_rect = rectangle(splash);
	targ.clear(sf::Color::Black);
	rect_draw_some_item(splash, from_rect, targ, dest_rect);
	targ.display();
}

static void rect_draw_some_item(const sf::Texture& src_gworld,rectangle src_rect,sf::RenderTarget& targ_gworld,rectangle targ_rect,sf::RenderStates mode,sf::Color colour = Colours::WHITE);

void rect_draw_some_item(sf::RenderTarget& targ_gworld,rectangle targ_rect) {
	fill_rect(targ_gworld, targ_rect, sf::Color::Black);
}

void rect_draw_some_item(const sf::Texture& src_gworld,rectangle src_rect,sf::RenderTarget& targ_gworld,rectangle targ_rect,sf::BlendMode mode, sf::Color colour){
	rect_draw_some_item(src_gworld, src_rect, targ_gworld, targ_rect, sf::RenderStates(mode), colour);
}

// I added this because I tried using clip_rect to fix missiles/booms and it didn't work.
void rect_draw_some_item(const sf::Texture& src_gworld,rectangle src_rect,sf::RenderTarget& targ_gworld,rectangle targ_rect,rectangle in_frame,sf::BlendMode mode,sf::Color colour){
	rectangle targ_clipped = targ_rect & in_frame;
	if(targ_clipped.empty()) return;
	rectangle src_clipped = src_rect;
	src_clipped.top += (targ_clipped.top - targ_rect.top);
	src_clipped.left += (targ_clipped.left - targ_rect.left);
	src_clipped.bottom += (targ_clipped.bottom - targ_rect.bottom);
	src_clipped.right += (targ_clipped.right - targ_rect.right);
	rect_draw_some_item(src_gworld, src_rect, targ_gworld, targ_clipped, sf::RenderStates(mode), colour);
}

// This is the static function that all the overrides call:
void rect_draw_some_item(const sf::Texture& src_gworld,rectangle src_rect,sf::RenderTarget& targ_gworld,rectangle targ_rect,sf::RenderStates mode, sf::Color colour) {
	rectangle src_gworld_rect(src_gworld), targ_gworld_rect(targ_gworld);
	src_rect &= src_gworld_rect;
	targ_rect &= targ_gworld_rect;
	if(src_rect.empty() || targ_rect.empty()) return;
	setActiveRenderTarget(targ_gworld);
	sf::Sprite tile(src_gworld, src_rect);
	tile.setPosition(targ_rect.left, targ_rect.top);
	tile.setColor(colour);
	double xScale = targ_rect.width(), yScale = targ_rect.height();
	xScale /= src_rect.width();
	yScale /= src_rect.height();
	tile.setScale(xScale, yScale);
	targ_gworld.draw(tile, mode);
}

void rect_draw_some_item(const sf::Texture& src_gworld,rectangle src_rect,const sf::Texture& mask_gworld,sf::RenderTarget& targ_gworld,rectangle targ_rect, sf::Color colour) {
	static sf::RenderTexture src;
	static bool inited = false;
	if(!inited || src_rect.width() != src.getSize().x || src_rect.height() != src.getSize().y) {
		src.create(src_rect.width(), src_rect.height());
		inited =  true;
	}
	rectangle dest_rect = src_rect;
	dest_rect.offset(-dest_rect.left,-dest_rect.top);
	rect_draw_some_item(src_gworld, src_rect, src, dest_rect);
	src.display();
	
	maskShader.setParameter("texture", sf::Shader::CurrentTexture);
	maskShader.setParameter("mask", mask_gworld);
	rect_draw_some_item(src.getTexture(), dest_rect, targ_gworld, targ_rect, &maskShader, colour);
}

std::map<sf::RenderTexture*,std::vector<ScaleAwareText>> store_scale_aware_text;
std::map<sf::RenderTexture*,rectangle> store_clip_rects;

static const bool DEBUG_SCALE_AWARE_TEXT = false;

static void draw_stored_scale_aware_text(sf::RenderTexture& texture, sf::RenderTarget& dest_window, rectangle targ_rect) {
	// Temporarily switch target window to its unscaled view to draw scale-aware text
	sf::View scaled_view = dest_window.getView();
	sf::Vector2f scaled_top_left = scaled_view_top_left(dest_window, scaled_view);
	dest_window.setView(dest_window.getDefaultView());
	std::vector<ScaleAwareText> stored_text = store_scale_aware_text[&texture];
	for(ScaleAwareText text : stored_text){
		sf::Text str_to_draw = text.text;
		if(!text.clip_rect.empty()){
			rectangle rect = text.clip_rect;
			rect.offset(targ_rect.left, targ_rect.top);
			rect.top *= get_ui_scale();
			rect.left *= get_ui_scale();
			rect.bottom *= get_ui_scale();
			rect.right *= get_ui_scale();
			rect.offset(scaled_top_left.x, scaled_top_left.y);
			// For debugging:
			if(DEBUG_SCALE_AWARE_TEXT)
				frame_rect(dest_window, rect, Colours::RED);
			clip_rect(dest_window, rect);
		}
		sf::Vector2f position = str_to_draw.getPosition();
		position = position + sf::Vector2f {0.0f+targ_rect.left, 0.0f+targ_rect.top};
		position *= (float)get_ui_scale();
		position += scaled_top_left;
		// Rounding to whole-number positions *might* avoid unpredictable graphics bugs:
		round_vec(position);
		str_to_draw.setPosition(position);
		dest_window.draw(str_to_draw);
		if(!text.clip_rect.empty())
			undo_clip(dest_window);
	}
	dest_window.setView(scaled_view);
}

void rect_draw_some_item(sf::RenderTexture& src_render_gworld,rectangle src_rect,sf::RenderTarget& targ_gworld,rectangle targ_rect,sf::BlendMode mode, sf::Color colour) {
	rect_draw_some_item(src_render_gworld.getTexture(), src_rect, targ_gworld, targ_rect, mode);
	draw_stored_scale_aware_text(src_render_gworld, targ_gworld, targ_rect);
}

void rect_draw_some_item(sf::RenderTexture& src_render_gworld,rectangle src_rect,const sf::Texture& mask_gworld,sf::RenderTarget& targ_gworld,rectangle targ_rect, sf::Color colour) {
	rect_draw_some_item(src_render_gworld.getTexture(), src_rect, mask_gworld, targ_gworld, targ_rect);
	draw_stored_scale_aware_text(src_render_gworld, targ_gworld, targ_rect);
}

void setActiveRenderTarget(sf::RenderTarget& where) {
	const std::type_info& type = typeid(where);
	if(type == typeid(sf::RenderWindow&))
		dynamic_cast<sf::RenderWindow&>(where).setActive();
	else if(type == typeid(sf::RenderTexture&))
		dynamic_cast<sf::RenderTexture&>(where).setActive();
	else throw std::bad_cast();
}
