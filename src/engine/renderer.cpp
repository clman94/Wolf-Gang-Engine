#define ENGINE_INTERNAL

#include <engine/renderer.hpp>

using namespace engine;

// ##########
// render_object
// ##########

int 
render_object::is_rendered()
{
	return mIndex >= 0;
}

void render_object::set_renderer(renderer& pR)
{
	pR.add_object(*this);
}

renderer* engine::render_object::get_renderer() const
{
	return mRenderer;
}

void render_object::detach_renderer()
{
	if (mRenderer)
	{
		mRenderer->remove_object(*this);
		mRenderer = nullptr;
	}
}

void
render_object::set_visible(bool pVisible)
{
	mVisible = pVisible;
}

bool
render_object::is_visible()
{
	return mVisible;
}

render_object::render_object()
{
	mIndex = -1;
	set_visible(true);
	mDepth = 0;
	mRenderer = nullptr;
}

render_object::~render_object()
{
	detach_renderer();
}

void
render_object::set_depth(depth_t pDepth)
{
	mDepth = pDepth;
	if (mRenderer)
		mRenderer->request_resort();
}

float
render_object::get_depth()
{
	return mDepth;
}

// ##########
// renderer
// ##########

renderer::renderer()
{
	mRequest_resort = false;
	mTgui.setWindow(mWindow);
}

renderer::~renderer()
{
	for (auto i : mObjects)
		i->mIndex = -1;
	close();
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

int
renderer::initualize(ivector pSize, int pFps)
{
	mWindow.create(sf::VideoMode(pSize.x, pSize.y), "Wolf-Gang Engine", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);
	if (pFps > 0)
		mWindow.setFramerateLimit(pFps);

	refresh_gui_view();
	return 0;
}


int
renderer::draw_objects()
{
	for (auto i : mObjects)
		if (i->is_visible())
			i->draw(*this);
	return 0;
}

int renderer::draw()
{
	mFrame_clock.update();
	if (mRequest_resort)
	{
		sort_objects();
		refresh_objects();
		mRequest_resort = false;
	}
	mWindow.clear(mBackground_color);
	draw_objects();
	mTgui.draw();
	mWindow.display();
	return 0;
}

int renderer::draw(render_object& pObject)
{
	return pObject.draw(*this);
}

tgui::Gui & renderer::get_tgui()
{
	return mTgui;
}

bool renderer::is_mouse_within_target() const
{
	const auto pos = get_mouse_position();
	const auto target = get_target_size();
	return pos.x >= 0  && pos.y >= 0 && pos.x < target.x && pos.y < target.y;
}

void renderer::refresh_view()
{
	const fvector window_size(
		static_cast<float>(mWindow.getSize().x)
		, static_cast<float>(mWindow.getSize().y));

	const fvector view_ratio = fvector(mTarget_size).normalize();
	const fvector window_ratio = fvector(window_size).normalize();

	sf::View view(sf::FloatRect(0, 0, mTarget_size.x, mTarget_size.y));
	sf::FloatRect viewport;


	// Resize view according to the ratio
	if (view_ratio.x > window_ratio.x)
	{
		viewport.width = 1;
		viewport.height = mTarget_size.y*(window_size.x / mTarget_size.x) / window_size.y;
	}
	else if (view_ratio.x < window_ratio.x)
	{
		viewport.width = mTarget_size.x*(window_size.y / mTarget_size.y) / window_size.x;
		viewport.height = 1;
	}
	else
	{
		viewport.width = 1;
		viewport.height = 1;
	}

	// Center view
	viewport.left = 0.5f - (viewport.width  / 2);
	viewport.top  = 0.5f - (viewport.height / 2);

	view.setViewport(viewport);
	mWindow.setView(view);
	return;
}

void renderer::refresh_gui_view()
{
	mTgui.setView(sf::View(sf::FloatRect(0, 0, static_cast<float>(mWindow.getSize().x), static_cast<float>(mWindow.getSize().y))));
}


void
renderer::refresh_objects()
{
	for (size_t i = 0; i < mObjects.size(); i++)
		mObjects[i]->mIndex = i;
}

// Sort items that have a higher depth to be farther behind
void
renderer::sort_objects()
{
	struct
	{
		bool operator()(render_object* c1, render_object* c2)
		{
			return c1->mDepth > c2->mDepth;
		}
	}client_sort;
	std::sort(mObjects.begin(), mObjects.end(), client_sort);
	refresh_objects();
}

int
renderer::add_object(render_object& pObject)
{
	// Already registered
	if (pObject.mRenderer == this)
		return 0;

	// Remove object from its previous renderer (if there is one)
	pObject.detach_renderer();

	pObject.mRenderer = this;
	pObject.mIndex = mObjects.size();
	mObjects.push_back(&pObject);
	pObject.refresh_renderer(*this);
	sort_objects();
	return 0;
}

int 
renderer::remove_object(render_object& pObject)
{
	// This is not the correct renderer to remove object from
	if (pObject.mRenderer != this)
		return 1;

	mObjects.erase(mObjects.begin() + pObject.mIndex);
	refresh_objects();
	pObject.mRenderer = nullptr;
	return 0;
}

int 
renderer::close()
{
	mWindow.close();
	return 0;
}

fvector
renderer::get_mouse_position() const
{
	auto pos = sf::Mouse::getPosition(mWindow);
	auto wpos = mWindow.mapPixelToCoords(pos);
	return wpos;
}

fvector
renderer::get_mouse_position(fvector pRelative) const
{
	return get_mouse_position() - pRelative;
}

bool
renderer::is_focused()
{
	return mWindow.hasFocus();
}



int renderer::set_icon(const std::string & pPath)
{
	sf::Image image;
	if (!image.loadFromFile(pPath))
		return 1;
	mWindow.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
	return 0;
}

int renderer::set_icon(const std::vector<char>& pData)
{
	sf::Image image;
	if (!image.loadFromMemory(&pData[0], pData.size()))
		return 1;
	mWindow.setIcon(image.getSize().x, image.getSize().y, image.getPixelsPtr());
	return 0;
}

void renderer::set_window_title(const std::string & pTitle)
{
	mWindow.setTitle(pTitle);
}

void
renderer::set_visible(bool pVisible)
{
	mWindow.setVisible(pVisible);
}

void
renderer::set_background_color(color pColor)
{
	mBackground_color = pColor;
}

float renderer::get_fps() const
{
	return mFrame_clock.get_fps();
}

float renderer::get_delta() const
{
	return mFrame_clock.get_delta();
}

void
renderer::refresh_pressed()
{
	for (auto &i : mPressed_keys)
	{
		if (i == 1)
			i = -1;
	}

	for (auto &i : mPressed_buttons)
	{
		if (i == 1)
			i = -1;
	}
}

bool
renderer::is_key_pressed(key_type pKey_type, bool pIgnore_gui)
{
	if (!mWindow.hasFocus() || (mIs_keyboard_busy && !pIgnore_gui))
		return false;
	bool is_pressed = sf::Keyboard::isKeyPressed(pKey_type);
	if (mPressed_keys[pKey_type] == -1 && is_pressed)
		return false;
	mPressed_keys[pKey_type] = is_pressed ? 1 : 0;
	return is_pressed;
}

bool
renderer::is_key_down(key_type pKey_type, bool pIgnore_gui)
{
	if (!mWindow.hasFocus() || (mIs_keyboard_busy && !pIgnore_gui))
		return false;
	return sf::Keyboard::isKeyPressed(pKey_type);
}

bool
renderer::is_mouse_pressed(mouse_button pButton_type, bool pIgnore_gui)
{
	if (!mWindow.hasFocus() || (mIs_mouse_busy && !pIgnore_gui) || !is_mouse_within_target())
		return false;
	bool is_pressed = sf::Mouse::isButtonPressed((sf::Mouse::Button)pButton_type);
	if (mPressed_buttons[pButton_type] == -1 && is_pressed)
		return false;
	mPressed_buttons[pButton_type] = is_pressed ? 1 : 0;
	return is_pressed;
}

bool
renderer::is_mouse_down(mouse_button pButton_type, bool pIgnore_gui)
{
	if (!mWindow.hasFocus() || (mIs_mouse_busy && !pIgnore_gui) || !is_mouse_within_target())
		return false;
	return sf::Mouse::isButtonPressed((sf::Mouse::Button)pButton_type);
}

int
renderer::update_events()
{
	refresh_pressed();

	if (!mWindow.isOpen())
		return 1;

	while (mWindow.pollEvent(mEvent))
	{
		if (mEvent.type == sf::Event::Closed)
			return 1;

		if (mEvent.type == sf::Event::Resized)
		{
			refresh_view();

			// Adjust view of gui so it won't stretch
			refresh_gui_view();
		}
		
		// Update tgui events
		{
			if (mTgui.handleEvent(mEvent))
			{
				mIs_mouse_busy = mEvent.type >= sf::Event::MouseWheelMoved
					&& mEvent.type <= sf::Event::MouseLeft;
				mIs_keyboard_busy = mEvent.type == sf::Event::KeyPressed
					|| mEvent.type == sf::Event::KeyReleased;
			}
			else
			{
				mIs_mouse_busy = false;
				mIs_keyboard_busy = false;
			}
		}
	}
	return 0;
}

/*
anchor_thing::anchor_thing()
{
	mAnchor_by = anchor_by::by_offset;
}

anchor_thing::anchor_thing(anchor pAnchor)
{
	mAnchor_by = anchor_by::by_anchor_point;
	mAnchor = pAnchor;
}

anchor_thing::anchor_thing(offset pOffset)
{
	mAnchor_by = anchor_by::by_offset;
	mPoint = pOffset;
}

fvector anchor_thing::calculate_offset()
{
	if (mAnchor_by == anchor_by::by_offset)
	{
		return mPoint;
	}
	return fvector();
}
*/

bool shader::load()
{
	if (!is_loaded())
	{
		if (!sf::Shader::isAvailable())
		{
			util::warning("Shaders are not supported on this platform");
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
	shape.setFillColor(sf::Color(c.r, c.g, c.b, c.a));
}

color rectangle_node::get_color()
{
	auto c = shape.getFillColor();
	return{ c.r, c.g, c.b, c.a };
}

void rectangle_node::set_size(fvector s)
{
	shape.setSize({ s.x, s.y });
}

fvector rectangle_node::get_size()
{
	return{ shape.getSize().x, shape.getSize().y };
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
	shape.setPosition({ pos.x, pos.y });
	if (mShader)
		pR.get_sfml_render().draw(shape, mShader->get_sfml_shader());
	else
		pR.get_sfml_render().draw(shape);
	return 0;
}
