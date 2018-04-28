#include <engine/primitive_builder.hpp>

using namespace engine;

void primitive_builder::push_offset(fvector pOffset)
{
	transform t;
	t.offset = pOffset;
	mTransform_stack.push_back(t);
}

void primitive_builder::push_scale(fvector pScale)
{
	transform t;
	t.scale = pScale;
	mTransform_stack.push_back(t);
}

void primitive_builder::push_rotation(float pRotation)
{
	transform t;
	t.rotation = pRotation;
	mTransform_stack.push_back(t);
}

void primitive_builder::push_node(const node & pNode)
{
	push_offset(pNode.get_exact_position());
	push_rotation(pNode.get_absolute_rotation());
	push_scale(pNode.get_absolute_scale());
}

void primitive_builder::pop_transform(std::size_t pCount)
{
	for (std::size_t i = 0; i < pCount; i++)
		mTransform_stack.pop_back();
}

void primitive_builder::pop_node()
{
	pop_transform(3);
}

void primitive_builder::add_circle(fvector pPosition, float pRadius, color pFill, color pOutline)
{
	std::vector<fvector> points;
	points.reserve(10);
	for (std::size_t i = 0; i < 10; i++)
		points.push_back((pPosition + fvector(pRadius, 0)).rotate(pPosition, (360.f/10.f)*(float)i));
	if (pFill.a > 0)
		add_poly(points, pFill);
	if (pOutline.a > 0)
		add_poly_lines(points, pOutline);
}

void primitive_builder::add_rectangle(frect pRect, color pFill, color pOutline)
{
	std::vector<fvector> points;
	for (std::size_t i = 0; i < 4; i++)
		points.push_back(pRect.get_vertex(i));
	if (pFill.a > 0)
		add_poly(points, pFill);
	if (pOutline.a > 0)
		add_poly_lines(points, pOutline);
}

void primitive_builder::add_line(fvector p0, fvector p1, color pColor)
{
	entry nentry;
	nentry.vertices.resize(2);
	nentry.vertices[0].color = pColor;
	nentry.vertices[0].position = p0;
	nentry.vertices[1].color = pColor;
	nentry.vertices[1].position = p1;
	apply_transform_stack(nentry.vertices);

	nentry.type = sf::PrimitiveType::Lines;

	if (!mEntries.empty()
		&& mEntries.back().type == nentry.type)
		mEntries.back().vertices.insert(mEntries.back().vertices.end(),
			nentry.vertices.begin(), nentry.vertices.end());
	else
		mEntries.push_back(nentry);
}

void primitive_builder::add_triangle(fvector p0, fvector p1, fvector p2, color pColor)
{
	entry nentry;
	nentry.vertices.resize(3);
	for (auto& i : nentry.vertices)
		i.color = pColor;
	nentry.vertices[0].position = p0;
	nentry.vertices[1].position = p1;
	nentry.vertices[2].position = p2;
	nentry.type = sf::PrimitiveType::Triangles;
	if (!mEntries.empty() && !mEntries.back().tex
		&& mEntries.back().type == nentry.type)
		mEntries.back().vertices.insert(mEntries.back().vertices.end(),
			nentry.vertices.begin(), nentry.vertices.end());
	else
		mEntries.push_back(nentry);
}

void primitive_builder::add_poly(std::vector<fvector> pPoints, color pFill, bool pLoop)
{
	assert(!pPoints.empty());
	entry nentry = generate_poly_entry(pPoints, pFill, pLoop);
	apply_transform_stack(nentry.vertices);
	nentry.type = sf::PrimitiveType::TriangleFan;
	mEntries.push_back(nentry);
}

void primitive_builder::add_poly_lines(std::vector<fvector> pPoints, color pOutline, bool pLoop)
{
	assert(!pPoints.empty());
	entry nentry = generate_poly_entry(pPoints, pOutline, pLoop);
	apply_transform_stack(nentry.vertices);
	nentry.type = sf::PrimitiveType::LineStrip;
	mEntries.push_back(nentry);
}

primitive_builder::handle primitive_builder::add_quad_texture(std::shared_ptr<texture> pTexture, frect pRect, frect pTexture_rect, color pTint)
{
	entry nentry;
	nentry.vertices.resize(6);
	for (std::size_t i = 0; i < 3; i++)
	{
		nentry.vertices[i] = sf::Vertex(pRect.get_vertex(i), pTint, pTexture_rect.get_vertex(i));
		nentry.vertices[i + 3] = sf::Vertex(pRect.get_vertex(i + 2), pTint, pTexture_rect.get_vertex(i + 2));
	}
	nentry.type = sf::PrimitiveType::Triangles;
	nentry.tex = pTexture;

	apply_transform_stack(nentry.vertices);

	// Combine with the last item if it draws the same texture.
	// This removes one draw call by simply putting them together.
	if (!mEntries.empty()
		&& mEntries.back().tex == pTexture
		&& mEntries.back().type == nentry.type)
		mEntries.back().vertices.insert(mEntries.back().vertices.end(),
			nentry.vertices.begin(), nentry.vertices.end());
	else
		mEntries.push_back(nentry);

	handle hnd;
	hnd.entry_index = mEntries.size() - 1;
	hnd.vertex_index = mEntries.back().vertices.size() - 6;
	return hnd;
}

primitive_builder::handle primitive_builder::add_quad_texture(std::shared_ptr<texture> pTexture, fvector pPosition, frect pTexture_rect, color pTint)
{
	return add_quad_texture(pTexture, frect(pPosition, pTexture_rect.get_size()), pTexture_rect, pTint);
}

primitive_builder::handle primitive_builder::add_quad_texture(std::shared_ptr<texture> pTexture, fvector pPosition, frect pTexture_rect, color pTint, int mRotate_corners)
{
	fvector offset = engine::frect({ 0, 0 }, pTexture_rect.get_size()).get_vertex(4 - (mRotate_corners % 4));
	push_offset(pPosition*get_unit() + (mRotate_corners % 2 ? offset.flip() : offset)); // Maintain top-left corner as positive
	push_rotation(mRotate_corners*90.f);
	auto hnd = add_quad_texture(pTexture, { { 0, 0 }, pTexture_rect.get_size() }, pTexture_rect, pTint);
	pop_transform(2);
	return hnd;
}

void primitive_builder::change_texture_rect(handle pHandle, frect pTexture_rect)
{
	entry& e = mEntries[pHandle.entry_index];

	for (std::size_t i = 0; i < 3; i++)
	{
		e.vertices[pHandle.vertex_index + i].texCoords = pTexture_rect.get_vertex(i);
		e.vertices[pHandle.vertex_index + 3 + i].texCoords = pTexture_rect.get_vertex(i + 2);
	}
}

void primitive_builder::clear()
{
	mEntries.clear();
}

int primitive_builder::draw(renderer & pR)
{
	sf::RenderStates rs = get_sfml_renderstates();
	for (const auto& i : mEntries)
	{
		if (i.tex)
			rs.texture = &i.tex->get_sfml_texture();
		else
			rs.texture = nullptr;
		pR.get_sfml_render().draw(&i.vertices[0], i.vertices.size(), i.type, rs);
	}
	return 0;
}

int primitive_builder::draw_and_clear(renderer & pR)
{
	int r = draw(pR);
	clear();
	return r;
}

fvector primitive_builder::apply_transform_stack(fvector pPoint) const
{
	fvector val = pPoint;
	for (std::size_t i = 0; i < mTransform_stack.size(); i++)
	{
		const transform& t = mTransform_stack[mTransform_stack.size() - i - 1];
		val += t.offset;
		if (t.rotation != 0)
			val.rotate(t.rotation);
		val *= t.scale;
	}
	return val;
}

void primitive_builder::apply_transform_stack(std::vector<sf::Vertex>& pVertices) const
{
	for (auto& i : pVertices)
		i.position = apply_transform_stack(i.position);
}

primitive_builder::entry primitive_builder::generate_poly_entry(std::vector<fvector> pPoints, color pColor, bool pLoop)
{
	assert(!pPoints.empty());
	entry nentry;
	for (const auto& i : pPoints)
	{
		sf::Vertex v;
		v.position = i;
		v.color = pColor;
		nentry.vertices.push_back(v);
	}
	if (pLoop)
		nentry.vertices.push_back(nentry.vertices[0]); // Connect back to start
	return nentry;
}

primitive_builder::transform::transform()
{
	offset = { 0, 0 };
	scale = { 1, 1 };
	rotation = 0;
}
