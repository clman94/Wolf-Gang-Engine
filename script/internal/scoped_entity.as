/// \weakgroup Entity
/// \{

/// Automatically removes entity when this object goes out of scope.
///
/// Ex.
/// ```
/// {
///    scoped_entity scopedentity = add_entity("foo");
/// } // scopedentity1 goes out of scope and the entity is deleted
/// ```
/// When assigning 2 scoped entities (`scopedentity1 = scopedentity2;`), the entity
/// referenced in scopedentity2 will _move_ to scopedentity1. So now scopedentity2 doesn't own
/// the entity any more and will not delete the entity when it goes out of scope.
///
/// Ex.
/// ```
/// scoped_entity scopedentity1;
/// {
///    scoped_entity scopedentity2 = add_entity("foo");
///    scopedentity1 = scopedentity2; // Entity moves to its new owner
/// } // scopedentity1 owns the entity now so scopedentity2 can't delete it.
/// ```
class scoped_entity
{
	scoped_entity()
	{}
	
	scoped_entity(scoped_entity@ pScoped_entity)
	{
		mEntity = pScoped_entity.mEntity;
		pScoped_entity.release();
	}

	scoped_entity(const string&in pTexture)
	{
		mEntity = add_entity(pTexture);
	}
	
	scoped_entity(const string&in pTexture, const string&in pAtlas)
	{
		mEntity = add_entity(pTexture, pAtlas);
	}
	
	scoped_entity(const entity&in pEntity)
	{
		mEntity = pEntity;
	}
	
	scoped_entity@ opAssign(const entity&in pEntity)
	{
		mEntity = pEntity;
		return this;
	}
	
	scoped_entity@ opAssign(scoped_entity@ pScoped_entity)
	{
		mEntity = pScoped_entity.mEntity;
		pScoped_entity.release();
		return this;
	}
	
	~scoped_entity()
	{
		if (mEntity.is_valid())
			remove_entity(mEntity);
	}
	
	/// Set the entity reference
	void set(const entity&in pEntity)
	{
		mEntity = pEntity;
	}
	
	/// Release the entity. It will not own the
	/// entity anymore.
	void release()
	{
		mEntity.release();
	}
	
	entity opImplConv() const
	{
		return mEntity;
	}
	
	private entity mEntity;
};

/// \}