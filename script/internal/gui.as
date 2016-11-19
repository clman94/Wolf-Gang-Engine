namespace gui
{
class group_item
{
	entity item_entity;
	string id;
	uint index;
};

class group
{
	group()
	{
		mLimit = 0;
	}
	
	uint get_size()
	{
		return mItems.length();
	}
	
	group_item@ get_item(uint pIndex)
	{
		if (mItems.length() == 0
		||  pIndex >= mItems.length())
			return group_item();
		return @mItems[pIndex];
	}
	
	uint add(entity pEntity, const string&in id)
	{
		group_item new_item;
		new_item.item_entity = pEntity;
		new_item.id = id;
		new_item.index = mItems.length();
		mItems.insertLast(new_item);
		return new_item.index;
	}
	
	void set_row_limit(uint pLimit)
	{
		mLimit = pLimit;
	}
	
	uint get_row_limit()
	{
		return mLimit;
	}
	
	private array<group_item> mItems;
	private uint mLimit;
};

class selector_control
{
	selector_control()
	{
		mCurrent_selection = 0;
	}
	
	group_item get_selection()
	{
		return mGroup.get_item(mCurrent_selection);
	}
	
	void set_group(group@ pGroup)
	{
		@mGroup = pGroup;
	}

	void tick()
	{
		int select = 0;
		
		// Check controls and calculate the offset for the new selection
		select -= is_triggered(control::select_previous) ? mGroup.get_row_limit() : 0;
		select += is_triggered(control::select_next)     ? mGroup.get_row_limit() : 0;
		select -= is_triggered(control::select_up)       ? 1 : 0;
		select += is_triggered(control::select_down)     ? 1 : 0;
		
		if (select != 0)
		{
			int new_selection = mCurrent_selection + select;
			
			// Prevent the selection from going outside the array.
			// Also gives a nice effect.
			if (new_selection >= 0 &&
				new_selection < int(mGroup.get_size()))
				mCurrent_selection = new_selection;
		}
	}
	private int mCurrent_selection;
	private group@ mGroup;
};

class list_selection
{
	list_selection(){
		mSelector.set_group(mGroup);
		mCurrent_group = 0;
		mColumn_seperation = 96;
		mGroup.set_row_limit(3);
	}
	
	void set_column_seperation(float pSeperation)
	{
		mColumn_seperation = pSeperation;
	}
	
	void set_row_limit(uint pRows)
	{
		mGroup.set_row_limit(pRows);
	}
	
	void add(const string&in pText, const string&in pId) final
	{
		entity new_selection = add_text();
		set_text(new_selection, pText);
		make_gui(new_selection, 1);
		::set_visible(new_selection, false);
		
		const vec position = position_text();
		
		::set_position(new_selection, position);
		mGroup.add(new_selection, pId);
		
		// Keep track of all the entities so they can
		// be deleted when this object is deleted.
		mEntities.insertLast(scoped_entity(new_selection));
	}
	
	void add(const string&in pText) final
	{
		add(pText, pText);
	}
	
	group_item@ start() final
	{
		set_visible(true);
		scoped_entity cursor = create_cursor();
		
		group_item@ current_item;
		
		do
		{
			mSelector.tick();
			
			@current_item = mSelector.get_selection();
			
			::set_position(cursor, get_position(current_item.item_entity));
			
			yield();
			
			if (is_triggered(control::back))
			{
				set_visible(false);
				return null;
			}
		}while(!is_triggered(control::activate));
		set_visible(false);
		return current_item;
	}
	
	void set_position(vec pPosition) final
	{
		mBase_position = pPosition;
	}
	
	protected entity create_cursor()
	{
		entity cursor = add_entity("NarrativeBox", "SelectCursor");
		set_anchor(cursor, anchor::topright);
		make_gui(cursor, 1);
		
		return cursor;
	}
	
	private vec position_text()
	{
		int row_count = mGroup.get_row_limit();
		
		return mBase_position 
			+ pixel(floor(mEntities.length()/row_count)*mColumn_seperation // Columns
			, 20 * (mEntities.length() % row_count)); // Max 3 lines
	}
	
	void set_visible(bool pIs_visible)
	{
		for (uint i = 0; i < mEntities.length(); i++)
		{
			::set_visible(mEntities[i], pIs_visible);
		}
	}
	
	private vec   mBase_position;
	private uint  mCurrent_group;
	private float mColumn_seperation;
	private group mGroup;
	private selector_control mSelector;
	private array<scoped_entity@> mEntities;
};

/// \weakgroup GUI
/// Menu system.
/// Because these gui classes are rather general use,
/// here are just a few uses:
/// ```
/// gui::selection_menu sel_options;
/// sel_action.add(gui::menu_function("Option1"
///     , function(pArgs) {
///         dprint("This is Option1");
///     }));
/// 
/// gui::selection_menu sel_submenu("Sub Menu");
/// sel_action.add(gui::menu_function("Sub Option1"
///     , function(pArgs) {
///         dprint("This is Sub Option1");
///     }));
/// sel_options.add(sel_submenu); // Add the sub-menu
/// 
/// sel_options.open(); // Start the selection
/// ```
/// 
/// \{

/// Base for a menu.
class menu
{
	menu(){}
	menu(const string&in pId)
	{
		mMenu_id = pId;
	}
	
	/// Open the menu
	/// Return null if there has been no selection (like going back).
	/// The menu before it will have to handle it correctly:
	/// ```
	/// menu@ ret;
	/// do{
	///   // Do its thing
	///   ret = sub_menu.open();
	/// } while(ret is null) // Keep looping until an item is selected
	///                      // or this function returns null and then
	///                      // the caller before it will have to handle this.
	/// return ret;
	/// ```
	menu@ open(){ return @this; }
	
	/// Get identification of menu
	string get_id()
	{
		return mMenu_id;
	}
	
	/// Set identification
	/// This string is usually used in the selection_menu
	/// for the text.
	void set_id(const string&in pId)
	{
		mMenu_id = pId;
	}
	
	private string mMenu_id;
};

class menu_item : menu
{
	menu_item()
	{
		mIs_selected = false;
	}
	
	menu_item(const string&in pId)
	{
		set_id(pId);
		mIs_selected = false;
	}
	
	menu@ open()
	{
		mIs_selected = true;
		return @this;
	}
	
	/// Check if this menu has been selected
	bool is_selected() const
	{
		return mIs_selected;
	}
	
	bool opImplConv() const
	{
		return mIs_selected;
	}
	
	private bool mIs_selected;
};

/// Function definition of callback

funcdef void menu_function_t(dictionary@);

/// Menu that calls a function when selected
/// 
/// Ex.
/// ```
/// void func1(dictionary@ pArgs)
/// {
///    dprint("This function is called");
/// }
/// 
/// gui::menu_function func1_menu("func1", func1); /// Simple call. pArgs will be null.
/// ```
/// Ex.
/// ```
/// void func2(dictionary@ pArgs)
/// {
///    string@ msg = cast<string>(pArgs["msg"]);
///    dprint(msg);
/// }
/// gui::menu_function func2_menu("func2"
///    , func2
///    , dictionary = { { "msg", "This function called" } }); /// Call with arguments represented by a dictionary type.
/// ```
class menu_function : menu_item
{
	menu_function(const string&in pId, menu_function_t@ pCallback)
	{
		super(pId);
		@mCallback = pCallback;
	}
	
	menu_function(const string&in pId, menu_function_t@ pCallback, dictionary@ pParam)
	{
		super(pId);
		@mCallback = pCallback;
		@mParam = pParam;
	}
	
	menu@ open()
	{
		if (mCallback !is null)
		{
			mCallback(mParam);
		}
		return @this;
	}
	
	private menu_function_t@ mCallback;
	private dictionary@ mParam;
};

/// Mixin for creating branches 
mixin class menu_branch
{
	/// Add a menu to the branch
	void add(menu@ pMenu) final
	{
		mSub_menus.insertLast(pMenu);
		menu_added(pMenu);
	}
	
	/// Remove a specific menu
	void remove(menu@ pMenu) final
	{
		for (uint i = 0; i < mSub_menus.length(); i++)
		{
			if (mSub_menus[i] is pMenu)
			{
				mSub_menus.removeAt(i);
				break;
			}
		}
		
		menu_removed(pMenu);
	}
	
	/// Remove all menus in branch
	void remove_all() final
	{
		mSub_menus.resize(0);
	}
	
	/// Get count of menus in branch
	uint count() final
	{
		return mSub_menus.length();
	}
	
	/// Get menu at index
	menu@ get_menu(uint pIndex) final
	{
		if (pIndex >= mSub_menus.length())
			return null;
		return mSub_menus[pIndex];
	}
	
	/// Callback when a new menu is added
	protected void menu_added(menu@ pMenu){ }
	
	/// Callback when a menu is deleted
	protected void menu_removed(menu@ pMenu){ }
	
	private array<menu@> mSub_menus;
};

/// Selection menu for a list of text
class selection_menu : menu, menu_branch
{
	selection_menu()
	{
		mCan_return = false;
	}
	selection_menu(const string&in pId)
	{
		super(pId);
		mCan_return = true;
	}
	
	protected void menu_added(menu@ pMenu) 
	{
		mSelector.add(pMenu.get_id());
	}
	
	/// Start selection
	menu@ open()
	{
		menu@ ret;
		do {
			group_item@ item = mSelector.start();
			if (item is null)
			{
				if (mCan_return)
					return null;
				else continue;
			}
			
			@ret = get_menu(item.index).open();
		} while(ret is null);
		return ret;
	}
	
	/// Set_ position of list
	void set_position(vec pPosition)
	{
		mSelector.set_position(pPosition);
	}
	
	/// Set row limit. When the row limit is reached,
	/// the menu items will overflow to the next column and
	/// then next vcolumn after that.
	void set_row_limit(uint pRows)
	{
		mSelector.set_row_limit(pRows);
	}
	
	/// Set whether of not the menu should be able to return
	/// to the menu before it without any selection.
	void set_return(bool pCan_return)
	{
		mCan_return =pCan_return;
	}
	
	private bool mCan_return;
	private list_selection mSelector;
};


/// \}
}






