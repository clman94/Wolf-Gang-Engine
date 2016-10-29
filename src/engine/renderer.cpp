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
	mTgui = nullptr;
}

renderer::~renderer()
{
	for (auto i : mObjects)
		i->mIndex = -1;
	close();
}

fvector
renderer::get_size()
{
	auto px = mWindow.getView().getSize();
	auto scale = mWindow.getView().getViewport();
	return
	{
		px.x / scale.width,
		px.y / scale.height
	};
}

void renderer::request_resort()
{
	mRequest_resort = true;
}

int
renderer::initualize(ivector pSize, int pFps)
{
	mWindow.create(sf::VideoMode(pSize.x, pSize.y), "Wolf-Gang Engine", sf::Style::Titlebar | sf::Style::Close);
	//mWindow.create(sf::VideoMode(pSize.x, pSize.y), "The Amazing Window", sf::Style::Fullscreen);
	if (pFps > 0)
		mWindow.setFramerateLimit(pFps);
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

int
renderer::draw()
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
	if (mTgui) mTgui->draw();
	mWindow.display();
	return 0;
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

void
renderer::set_pixel_scale(float pScale)
{
	sf::View view = mWindow.getDefaultView();
	sf::FloatRect fr = view.getViewport();
	fr.width  *= pScale;
	fr.height *= pScale;
	view.setViewport(fr);
	mWindow.setView(view);
}

fvector
renderer::get_mouse_position()
{
	auto pos = sf::Mouse::getPosition(mWindow);
	auto wpos = mWindow.mapPixelToCoords(pos);
	return wpos;
}

fvector
renderer::get_mouse_position(fvector pRelative)
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

float renderer::get_fps()
{
	return mFrame_clock.get_fps();
}

float renderer::get_delta()
{
	return mFrame_clock.get_delta();
}

void renderer::set_gui(tgui::Gui * pTgui)
{
	mTgui = pTgui;
	pTgui->setWindow(mWindow);
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
renderer::is_key_pressed(key_type pKey_type)
{
	if (!mWindow.hasFocus())
		return false;
	bool is_pressed = sf::Keyboard::isKeyPressed(pKey_type);
	if (mPressed_keys[pKey_type] == -1 && is_pressed)
		return false;
	mPressed_keys[pKey_type] = is_pressed ? 1 : 0;
	return is_pressed;
}

bool
renderer::is_key_down(key_type pKey_type)
{
	if (!mWindow.hasFocus())
		return false;
	return sf::Keyboard::isKeyPressed(pKey_type);
}

bool
renderer::is_mouse_pressed(mouse_button pButton_type)
{
	if (!mWindow.hasFocus())
		return false;
	bool is_pressed = sf::Mouse::isButtonPressed((sf::Mouse::Button)pButton_type);
	if (mPressed_buttons[pButton_type] == -1 && is_pressed)
		return false;
	mPressed_buttons[pButton_type] = is_pressed ? 1 : 0;
	return is_pressed;
}

bool
renderer::is_mouse_down(mouse_button pButton_type)
{
	if (!mWindow.hasFocus())
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
		
		if (mTgui)
			mTgui->handleEvent(mEvent);
	}
	return 0;
}

