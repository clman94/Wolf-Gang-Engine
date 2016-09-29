#include <engine/renderer.hpp>

using namespace engine;

// ##########
// render_client
// ##########

int 
render_client::is_rendered()
{
	return mIndex >= 0;
}

void render_client::set_renderer(renderer& pR)
{
	pR.add_client(*this);
}

void render_client::detach_renderer()
{
	if (mRenderer)
		mRenderer->remove_client(*this);
}

void
render_client::set_visible(bool pVisible)
{
	mVisible = pVisible;
}

bool
render_client::is_visible()
{
	return mVisible;
}

render_client::render_client()
{
	mIndex = -1;
	set_visible(true);
	mDepth = 0;
	mRenderer = nullptr;
}

render_client::~render_client()
{
	detach_renderer();
}

void
render_client::set_depth(depth_t pDepth)
{
	mDepth = pDepth;
	if (mRenderer)
		mRenderer->request_resort();
}

float
render_client::get_depth()
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
	for (auto i : mClients)
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
renderer::draw_clients()
{
	for (auto i : mClients)
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
		sort_clients();
		refresh_clients();
		mRequest_resort = false;
	}
	mWindow.clear(mBackground_color);
	draw_clients();
	if (mTgui) mTgui->draw();
	mWindow.display();
	return 0;
}

void
renderer::refresh_clients()
{
	for (size_t i = 0; i < mClients.size(); i++)
		mClients[i]->mIndex = i;
}

// Sort items that have a higher depth to be farther behind
void
renderer::sort_clients()
{
	struct
	{
		bool operator()(render_client* c1, render_client* c2)
		{
			return c1->mDepth > c2->mDepth;
		}
	}client_sort;
	std::sort(mClients.begin(), mClients.end(), client_sort);
	refresh_clients();
}

int
renderer::add_client(render_client& pClient)
{
	if (pClient.mIndex >= 0)
	{
		return remove_client(pClient);
	}
	pClient.mRenderer = this;
	pClient.mIndex = mClients.size();
	mClients.push_back(&pClient);
	pClient.refresh_renderer(*this);
	sort_clients();
	return 0;
}

int 
renderer::remove_client(render_client& pClient)
{
	if (pClient.mIndex < 0
	||  pClient.mRenderer != this)
	{
		return 1;
	}
	mClients.erase(mClients.begin() + pClient.mIndex);
	refresh_clients();
	pClient.mIndex = -1;
	pClient.mRenderer = nullptr;
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

void
renderer::refresh_pressed()
{
	for (auto &i : mPressed_keys)
	{
		if (i.second == 1)
			i.second = -1;
	}

	for (auto &i : mPressed_buttons)
	{
		if (i.second == 1)
			i.second = -1;
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

