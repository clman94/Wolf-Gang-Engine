#define ENGINE_INTERNAL
#include <imgui.h>
#include <imgui-SFML.h>

#include <engine/renderer.hpp>
#include <engine/logger.hpp>

#include <algorithm>

using namespace engine;

// ##########
// render_object
// ##########

void render_object::set_renderer(renderer& pR, bool pManual_render)
{
	mManual_render = pManual_render;
	if (pManual_render)
	{
		mRenderer = &pR;
		refresh_renderer(pR);
		mVisible = true;
	}
	else
		pR.add_object(*this);
}

renderer* render_object::get_renderer() const
{
	return mRenderer;
}

void render_object::detach_renderer()
{
	if (mRenderer)
	{
		if (!mManual_render)
			mRenderer->remove_object(*this);
		mRenderer = nullptr;
	}
}

sf::RenderStates render_object::get_sfml_renderstates() const
{
	sf::RenderStates rs;
	rs.transform.translate(get_exact_position());
	rs.transform.rotate(get_absolute_rotation());
	rs.transform.scale(get_absolute_scale());
	return rs;
}

void render_object::set_visible(bool pVisible)
{
	mVisible = pVisible;
}

bool render_object::is_visible()
{
	return mVisible;
}

render_object::render_object()
{
	mManual_render = false;
	mVisible = true;
	mDepth = 0;
	mRenderer = nullptr;
}

render_object::~render_object()
{
	detach_renderer();
}

void render_object::set_depth(float pDepth)
{
	mDepth = pDepth;
	if (mRenderer)
		mRenderer->request_resort();
}

float render_object::get_depth()
{
	return mDepth;
}

// ##########
// renderer
// ##########

renderer::renderer()
{
	mWindow = nullptr;
	mRender_target = nullptr;
	mRequest_resort = false;
	mTarget_size = fvector(800, 600); // Some arbitrary default
}

renderer::~renderer()
{
	for (auto i : mObjects)
		i->mRenderer = nullptr;
}

void renderer::set_target_size(fvector pSize)
{
	mTarget_size = pSize;
	refresh_view();
}

fvector renderer::get_target_size() const
{
	return mTarget_size;
}

void renderer::request_resort()
{
	mRequest_resort = true;
}

int renderer::draw_object_with_shader()
{
	fvector size = fvector(mRender_target->getSize())*fvector(mView.getViewport().width, mView.getViewport().height);
	if (mTemp_shader_buffer.getSize() != sf::Vector2u(size))
		mTemp_shader_buffer.create(size.x, size.y);
	mTemp_shader_buffer.setView(sf::View(sf::FloatRect(0, 0, mTarget_size.x, mTarget_size.y)));
	mRender_target->setView(mRender_target->getDefaultView());
	mShader->set_uniform("target_size", vector_cast<int>(mTarget_size));

	// Draw the objects affected by this shader
	std::size_t i = 0;
	for (; i < mObjects.size() && mObjects[i]->get_depth() > mShader_depth; i++)
		if (mObjects[i]->is_visible())
			mObjects[i]->draw(*this);

	sf::Sprite sprite(mTemp_shader_buffer.getTexture());
	sprite.setPosition((sf::Vector2f(mRender_target->getSize()) / 2.f) - sf::Vector2f(size / 2.f));

	// Draw the buffer to the window
	mTemp_shader_buffer.display();
	mRender_target->draw(sprite, mShader->get_sfml_shader());

	// Draw the unaffected
	mTemp_shader_buffer.clear({ 0, 0, 0, 0 });
	for (; i < mObjects.size(); i++)
		if (mObjects[i]->is_visible())
			mObjects[i]->draw(*this);

	// Draw the buffer (again) to the window
	mTemp_shader_buffer.display();
	mRender_target->draw(sprite);
	return 0;
}

int renderer::draw_objects()
{
	for (auto i : mObjects)
	{
		if (i->is_visible())
		{
			// mWindow->mWindow.setView(mView);
			i->draw(*this);
		}
	}
	return 0;
}

int renderer::draw(bool pClear)
{
	mFrame_clock.update();
	if (mRequest_resort)
	{
		sort_objects();
		mRequest_resort = false;
	}
	if (pClear)
	{
		mRender_target->clear(mBackground_color);
		if (mShader)
			mTemp_shader_buffer.clear(mBackground_color);
	}

	if (mShader)
	{
		draw_object_with_shader();
	}
	else
	{
		mRender_target->setView(mView);
		draw_objects();
	}

	return 0;
}

int renderer::draw(render_object& pObject)
{
	if (mWindow)
		mWindow->mWindow.setView(mView);
	return pObject.draw(*this);
}


std::string renderer::get_entered_text() const
{
	std::string ascii;

	// Convert to ascii
	for (const auto& i : mEntered_text)
		ascii += static_cast<char>(i < 128 ? i : '?');

	return ascii;
}

std::u32string renderer::get_entered_text_unicode() const
{
	return mEntered_text;
}

void renderer::set_shader(std::shared_ptr<shader> pShader, float pDepth)
{
	mShader = pShader;
	mShader_depth = pDepth;
}

bool renderer::is_mouse_within_target() const
{
	const auto pos = get_mouse_position();
	const auto target = get_target_size();
	return pos.x >= 0  && pos.y >= 0 && pos.x < target.x && pos.y < target.y;
}

void renderer::refresh_view()
{
	if (!mRender_target)
		return;
	const fvector window_size(vector_cast<float, unsigned int>(mRender_target->getSize()));

	mView.reset(sf::FloatRect(0, 0, mTarget_size.x, mTarget_size.y));

	sf::FloatRect viewport(0, 0, 0, 0);

	// Fit in window while maintaining aspect ratio
	viewport.width = std::min(mTarget_size.x*(window_size.y / mTarget_size.y) / window_size.x, 1.f);
	viewport.height = std::min(mTarget_size.y*(window_size.x / mTarget_size.x) / window_size.y, 1.f);

	// Center it
	viewport.left = 0.5f - (viewport.width  / 2);
	viewport.top  = 0.5f - (viewport.height / 2);

	mView.setViewport(viewport);
}

// Sort items that have a higher depth to be farther behind
void renderer::sort_objects()
{
	std::sort(mObjects.begin(), mObjects.end(),
		[](render_object* c1, render_object* c2)->bool
		{
			return c1->mDepth > c2->mDepth;
		});
}

bool renderer::add_object(render_object& pObject)
{
	// Already registered
	if (pObject.mRenderer == this)
		return false;

	// Remove object from its previous renderer (if there is one)
	pObject.detach_renderer();

	mObjects.push_back(&pObject);

	pObject.mRenderer = this;
	pObject.refresh_renderer(*this);

	sort_objects();
	return true;
}

bool renderer::remove_object(render_object& pObject)
{
	// This is not the correct renderer to remove object from
	if (pObject.mRenderer != this)
		return false;

	// Find item with the same pointer
	auto iter = std::find(mObjects.begin(), mObjects.end(), &pObject);
	if (iter == mObjects.end())
		return false;

	mObjects.erase(iter);
	pObject.mRenderer = nullptr;
	return true;
}

fvector renderer::get_mouse_position() const
{
	return mRender_target->mapPixelToCoords(mMouse_position, mView);
}

fvector renderer::get_mouse_position(fvector pRelative) const
{
	return get_mouse_position() - pRelative;
}

fvector renderer::get_mouse_position(const node & pNode) const
{
	const fvector pos = pNode.get_exact_position();
	const fvector scale = pNode.get_absolute_scale();
	if (scale.has_zero())
		return fvector(0, 0);
	return (get_mouse_position(pos)).rotate(pos, -pNode.get_absolute_rotation())/scale;
}

fvector renderer::window_to_game_coords(ivector pPixels) const
{
	return mRender_target->mapPixelToCoords(pPixels, mView);
}

fvector renderer::window_to_game_coords(ivector pPixels, const fvector & pRelative) const
{
	return window_to_game_coords(pPixels) - pRelative;
}

fvector renderer::window_to_game_coords(ivector pPixels, const node & pNode) const
{
	const fvector pos = pNode.get_exact_position();
	const fvector scale = pNode.get_absolute_scale();
	if (scale.has_zero())
		return fvector(0, 0); // TODO: Should have some sort of error (possibly an exception)
	return (window_to_game_coords(pPixels, pos).rotate(pos, -pNode.get_absolute_rotation()) / scale) / pNode.get_unit();
}

ivector renderer::game_to_window_coords(fvector pPosition) const
{
	return mRender_target->mapCoordsToPixel(pPosition, mView);
}

ivector renderer::game_to_window_coords(fvector pPosition, const node & pNode) const
{
	return game_to_window_coords(((pPosition*pNode.get_absolute_scale()).rotate(pNode.get_absolute_rotation())
		+ pNode.get_absolute_position())*pNode.get_unit());
}

bool renderer::is_focused()
{
	return mWindow->mWindow.hasFocus();
}

void renderer::set_visible(bool pVisible)
{
	mWindow->mWindow.setVisible(pVisible);
}

void renderer::set_background_color(color pColor)
{
	mBackground_color = pColor;
}

const color & engine::renderer::get_background_color() const
{
	return mBackground_color;
}

float renderer::get_fps() const
{
	return mFrame_clock.get_fps();
}

float renderer::get_delta() const
{
	return mFrame_clock.get_delta();
}

void renderer::set_window(display_window & pWindow)
{
	mWindow = &pWindow;
	set_target_render(mWindow->mWindow);
}

display_window * renderer::get_window() const
{
	return mWindow;
}

void renderer::set_target_render(sf::RenderTarget & pTarget)
{
	mRender_target = &pTarget;
}

void renderer::refresh()
{
	refresh_view();
}

sf::RenderTarget & renderer::get_sfml_render()
{
	assert(mRender_target);
	if (mShader)
		return mTemp_shader_buffer;
	return *mRender_target;
}

void renderer::refresh_input()
{
	for (auto &i : mPressed_keys)
	{
		if (i == input_state::pressed)
			i = input_state::hold;
		else if (i == input_state::released)
			i = input_state::none;
	}

	for (auto &i : mPressed_buttons)
	{
		if (i == input_state::pressed)
			i = input_state::hold;
		else if (i == input_state::released)
			i = input_state::none;
	}
}

bool renderer::is_key_pressed(key_code pKey_type, bool pIgnore_gui)
{
	if (mWindow && !mWindow->mWindow.hasFocus())
		return false;
	return mPressed_keys[pKey_type] == input_state::pressed;
}

bool renderer::is_key_down(key_code pKey_type, bool pIgnore_gui)
{
	if (mWindow && !mWindow->mWindow.hasFocus())
		return false;
	return mPressed_keys[pKey_type] == input_state::pressed
		|| mPressed_keys[pKey_type] == input_state::hold;
}

bool renderer::is_mouse_pressed(mouse_button pButton_type, bool pIgnore_gui)
{
	if ((mWindow && !mWindow->mWindow.hasFocus()) || !is_mouse_within_target())
		return false;
	return mPressed_buttons[pButton_type] == input_state::pressed;
}

bool renderer::is_mouse_down(mouse_button pButton_type, bool pIgnore_gui)
{
	if (mWindow && !mWindow->mWindow.hasFocus() || !is_mouse_within_target())
		return false;
	return mPressed_buttons[pButton_type] == input_state::pressed
		|| mPressed_buttons[pButton_type] == input_state::hold;
}

void renderer::update_events()
{
	assert(mWindow);
	update_events(*mWindow);
}

void renderer::update_events(display_window& pWindow)
{
	refresh_input();

	if (!pWindow.mWindow.isOpen())
		return;

	mEntered_text.clear();

	for (const sf::Event& i : pWindow.mEvents)
	{
		if (i.type == sf::Event::Resized)
		{
			pWindow.mSize = vector_cast<int, unsigned int>(pWindow.mWindow.getSize()); // Update member
			refresh_view();
		}
		if (i.type == sf::Event::TextEntered)
			mEntered_text += static_cast<char32_t>(i.text.unicode);

		// Key events
		if (i.type == sf::Event::KeyPressed && (size_t)i.key.code < mPressed_keys.size())
			mPressed_keys[(size_t)i.key.code] = input_state::pressed;
		else if (i.type == sf::Event::KeyReleased && (size_t)i.key.code < mPressed_keys.size())
			mPressed_keys[(size_t)i.key.code] = input_state::released;


		if (i.type == sf::Event::MouseWheelMoved)
		{
			// TODO
		}

		// Mouse events
		if (i.type == sf::Event::MouseButtonPressed)
			mPressed_buttons[(size_t)i.mouseButton.button] = input_state::pressed;
		else if (i.type == sf::Event::MouseButtonReleased)
			mPressed_buttons[(size_t)i.mouseButton.button] = input_state::released;

		if (i.type == sf::Event::MouseMoved)
			mMouse_position = { i.mouseMove.x, i.mouseMove.y };
	}
}

void renderer::update_events(display_window & pWindow, engine::fvector pMouse_position)
{
	update_events(pWindow);
	mMouse_position = vector_cast<int>(pMouse_position);
}

bool shader::load()
{
	if (!is_loaded())
	{
		if (!sf::Shader::isAvailable())
		{
			logger::warning("Shaders are not supported on this platform");
			return false;
		}

		mSFML_shader.reset(new sf::Shader());

		bool success = false;

		if (mVertex_shader_path.empty())
		{
			success = mSFML_shader->loadFromFile(mFragment_shader_path, sf::Shader::Fragment); // Load only fragment
		}
		else if (mFragment_shader_path.empty())
		{
			success = mSFML_shader->loadFromFile(mVertex_shader_path, sf::Shader::Vertex); // Load only vertex
		}
		else
		{
			success = mSFML_shader->loadFromFile(mVertex_shader_path, mFragment_shader_path); // Load both
		}
		if (success)
		{
			mSFML_shader->setUniform("texture", sf::Shader::CurrentTexture);
		}

		set_loaded(success);
		return success;
	}
	return false;
}

bool shader::load(const std::string& pVert, const std::string& pFrag)
{
	mSFML_shader.reset(new sf::Shader());
	bool succ = mSFML_shader->loadFromMemory(pVert, pFrag);
	mSFML_shader->setUniform("texture", sf::Shader::CurrentTexture);
	return succ;
}

bool shader::unload()
{
	mSFML_shader.reset();
	return true;
}

void shader::set_vertex_path(const std::string & pPath)
{
	mVertex_shader_path = pPath;
}

void shader::set_fragment_path(const std::string & pPath)
{
	mFragment_shader_path = pPath;
}

void shader::set_uniform(const std::string & pName, int pVal)
{
	mSFML_shader->setUniform(pName, pVal);
}

void shader::set_uniform(const std::string & pName, float pVal)
{
	mSFML_shader->setUniform(pName, pVal);
}

void shader::set_uniform(const std::string & pName, ivector pVal)
{
	mSFML_shader->setUniform(pName, sf::Glsl::Ivec2(pVal.x, pVal.y));
}

void shader::set_uniform(const std::string & pName, fvector pVal)
{
	mSFML_shader->setUniform(pName, sf::Glsl::Vec2(pVal.x, pVal.y));
}

void shader::set_uniform(const std::string & pName, color pVal)
{
	mSFML_shader->setUniform(pName, sf::Glsl::Vec3(pVal.r, pVal.g, pVal.b));
}

render_proxy::render_proxy() : mR(nullptr)
{
}

void render_proxy::set_renderer(renderer & pR)
{
	mR = &pR;
	refresh_renderer(pR);
}

renderer * render_proxy::get_renderer()
{
	return mR;
}

rectangle_node::rectangle_node()
{
	mAnchor = anchor::topleft;
}

void rectangle_node::set_anchor(anchor pAnchor)
{
	mAnchor = pAnchor;
}

void rectangle_node::set_color(const color & c)
{
	shape.setFillColor(c);
}

color rectangle_node::get_color()
{
	return shape.getFillColor();
}

void rectangle_node::set_size(fvector s)
{
	shape.setSize(s);
}

fvector rectangle_node::get_size() const
{
	return shape.getSize();
}

void rectangle_node::set_outline_color(color pColor)
{
	shape.setOutlineColor(pColor);
}

void rectangle_node::set_outline_thinkness(float pThickness)
{
	shape.setOutlineThickness(pThickness);
}

int rectangle_node::draw(renderer & pR)
{
	auto pos = get_exact_position() + engine::anchor_offset(get_size(), mAnchor);
	shape.setPosition(pos);
	shape.setRotation(get_absolute_rotation());
	shape.setScale(get_absolute_scale());

	if (mShader)
		pR.get_sfml_render().draw(shape, mShader->get_sfml_shader());
	else
		pR.get_sfml_render().draw(shape);
	return 0;
}

frect rectangle_node::get_render_rect() const
{
	return{ get_exact_position() + engine::anchor_offset(get_size(), mAnchor), get_size() };
}

display_window::display_window(const std::string & pTitle, ivector pSize)
{
	initualize(pTitle, pSize);
	mIs_resizing = false;
}

display_window::~display_window()
{
	mWindow.close();
}

void display_window::initualize(const std::string& pTitle, ivector pSize)
{
	mTitle = pTitle;
	mSize = pSize;
	mIs_fullscreen = false;
	windowed_mode();
}

void display_window::set_size(ivector pSize)
{
	mSize = pSize;
	if (!mIs_fullscreen)
		mWindow.setSize({ static_cast<unsigned int>(pSize.x), static_cast<unsigned int>(pSize.y) });
}

ivector display_window::get_size() const
{
	return mSize;
}

void display_window::set_position(engine::fvector pPosition)
{
	mWindow.setPosition(pPosition);
}

fvector display_window::get_position() const
{
	return mWindow.getPosition();
}

void display_window::windowed_mode()
{
	mWindow.create(sf::VideoMode(mSize.x, mSize.y), mTitle, sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
	mIs_fullscreen = false;
}

void display_window::toggle_mode()
{
	if (mIs_fullscreen)
		windowed_mode();
	else
		fullscreen_mode();
}

bool display_window::is_fullscreen() const
{
	return mIs_fullscreen;
}


void display_window::set_title(const std::string & pTitle)
{
	mWindow.setTitle(pTitle);
}

int display_window::set_icon(const std::string & pPath)
{
	sf::Image image;
	if (!image.loadFromFile(pPath))
		return 1;
	mWindow.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
	return 0;
}

int display_window::set_icon(const std::vector<char>& pData)
{
	sf::Image image;
	if (!image.loadFromMemory(&pData[0], pData.size()))
		return 1;
	mWindow.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
	return 0;
}

bool display_window::is_open() const
{
	return mWindow.isOpen();
}

bool display_window::poll_events()
{
	mEvents.clear();
	sf::Event event;
	mIs_resizing = false;
	while (mWindow.pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
			return false;
		if (event.type == sf::Event::Resized)
		{
			mSize = mWindow.getSize();
			mIs_resizing = true;
		}
		if (event.type == sf::Event::LostFocus)
			mWindow.setFramerateLimit(15);
		if (event.type == sf::Event::GainedFocus)
			mWindow.setFramerateLimit(60);
		mEvents.push_back(event);
	}
	return true;
}

void display_window::display()
{
	mWindow.display();
}

void display_window::clear(color mColor)
{
	mWindow.clear(mColor);
}

void display_window::push_events_to_imgui() const
{
	for (const auto& i : mEvents)
		ImGui::SFML::ProcessEvent(i);
}

sf::RenderWindow& display_window::get_sfml_window()
{
	return mWindow;
}

bool display_window::is_resizing() const
{
	return mIs_resizing;
}

void display_window::fullscreen_mode()
{
	mWindow.create(sf::VideoMode::getDesktopMode(), mTitle, sf::Style::None);
	mIs_fullscreen = true;
}