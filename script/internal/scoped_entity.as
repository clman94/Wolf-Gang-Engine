
// Removes entity when this object goes out of scope
class scoped_entity
{
	scoped_entity()
	{}
	
	scoped_entity(const scoped_entity&in p)
	{
		mEntity = p;
	}

	scoped_entity(const string&in pTexture)
	{
		mEntity = add_entity(pTexture);
	}
	
	scoped_entity(const string&in pTexture, const string&in pAtlas)
	{
		mEntity = add_entity(pTexture, pAtlas);
	}
	
	scoped_entity(entity pEntity)
	{
		mEntity = pEntity;
	}
	
	scoped_entity@ opAssign(const entity&in pEntity)
	{
		mEntity = pEntity;
		return this;
	}
	
	scoped_entity@ opAssign(const scoped_entity&in p)
	{
		mEntity = p;
		return this;
	}
	
	~scoped_entity()
	{
		if (mEntity.is_valid())
			remove_entity(mEntity);
	}
	
	void release()
	{
		// TODO
	}
	
	entity opImplConv() const
	{
		return mEntity;
	}
	
	private entity mEntity;
};