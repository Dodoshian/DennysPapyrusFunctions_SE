
using namespace std;
using namespace RE;

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
#ifndef DEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}


	*path /= Version::PROJECT;
	*path += ".log"sv;
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef DEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::info);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

/*
Takes a Weapon object as argument.
Returns its weapon template (the weapon it uses as a base in Creation Kit). 

If there is no template then it returns None.
*/
RE::TESObjectWEAP* GetTemplateWeapon(RE::StaticFunctionTag*, RE::TESObjectWEAP* baseWeapon)
{
	//const char *name = baseWeapon->GetName();

	// logger::info("Running Function");
	// logger::info("source Weapon is {}", name);
	RE::TESObjectWEAP* templateWeapon = baseWeapon->templateWeapon;
	if (!templateWeapon) {
		// templateWeapon = baseWeapon;
		templateWeapon = nullptr;
	}
	return templateWeapon;
	// TESForm *weep = RE::TESForm::LookupByID(0x00012EB7);
	//  TESObjectWEAP *toReturn = static_cast<TESObjectWEAP *>(weep);
	//  return toReturn;
}

/*
Takes a Armor object as argument.
Returns its Armor template (the Armor it uses as a base in Creation Kit).

If there is no template then it returns None.
*/
RE::TESObjectARMO* GetTemplateArmor(RE::StaticFunctionTag*, RE::TESObjectARMO* baseArmor)
{
	// const char *name = baseArmor->GetName();

	// logger::info("Running Function");
	// logger::info("source Armor is {}", name);
	RE::TESObjectARMO* templateArmor = baseArmor->templateArmor;
	if (!templateArmor) {
		// templateArmor = baseArmor;
		templateArmor = nullptr;
	}
	return templateArmor;
}

/*Returns the tempering recipe of a Weapon, if it exists. 
* If the weapon is based on a template weapon, then that weapon is checked for a recipe and it is returned.

Otherwise returns nullptr.*/
RE::BGSConstructibleObject* GetWeaponTemperingRecipe(RE::StaticFunctionTag*, RE::TESObjectWEAP* baseWeapon)
{
	logger::info("running code");
	RE::TESForm* temperKeyword = TESForm::LookupByEditorID("CraftingSmithingSharpeningWheel");
	RE::TESObjectWEAP* tWeapon = baseWeapon->templateWeapon;
	if (!tWeapon) {
		tWeapon = baseWeapon;
	}

	RE::BSTArray<RE::BGSConstructibleObject*> thisArray =
		RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSConstructibleObject>();

	int i = 0;
	for (BGSConstructibleObject* item : thisArray) {
		// logger::info("Current item: {}", i);
		i += 1;
		if (item->createdItem == tWeapon)  //if this recipe creates the same item we have here...
		{
			if (item->benchKeyword == temperKeyword)  // and the keyword is grindstone
			{
				//        logger::info("This is the recipe for {}", tWeapon->GetName());
				return item;
			}
		}
	}
	return nullptr;
}

/*Returns the tempering recipe of an Armor, if it exists.
* If the Armor is based on a template armor, then that armor is checked for a recipe and it is returned.

Otherwise returns nullptr.*/
RE::BGSConstructibleObject* GetArmorTemperingRecipe(RE::StaticFunctionTag*, RE::TESObjectARMO* baseArmor)
{
	// logger::info("running code");
	RE::TESForm* temperKeyword = TESForm::LookupByEditorID("CraftingSmithingArmorTable");
	RE::TESObjectARMO* tArmor = baseArmor->templateArmor;
	if (!tArmor) {
		tArmor = baseArmor;
	}

	RE::BSTArray<RE::BGSConstructibleObject*> thisArray =
		RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSConstructibleObject>();

	int i = 0;
	for (BGSConstructibleObject* item : thisArray) {
		//  logger::info("Current item: {}", i);
		i += 1;
		if (item->createdItem == tArmor)  // if this recipe creates the same item we have here...
		{
			if (item->benchKeyword == temperKeyword)  // and the keyword is armor bench
			{
				//        logger::info("This is the recipe for {}", tArmor->GetName());
				return item;
			}
		}
	}
	return nullptr;
}

/*
Returns the Recipe required to make this item at the forge, if it exists..

Note this will not work for weapons and armor that use template weapons or armors.
Make sure you use GetTemplateArmor() or GetTemplateWeapon() first in those cases.

By Default, this will check any workbench's recipes. 
To check a specific workbench, pass its keyword in as a string as argument 2.

*/
RE::BGSConstructibleObject* GetCraftingRecipe(RE::StaticFunctionTag*, RE::TESForm* baseForm, string keywordString)
{
	// logger::info("running code");
	RE::TESForm* temperKeyword = TESForm::LookupByEditorID(keywordString);

	RE::BSTArray<RE::BGSConstructibleObject*> thisArray =
		RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSConstructibleObject>();
	size_t len = keywordString.length();
	int i = 0;

	if (len == 0)  // if arg 2 is empty string
	{
		for (BGSConstructibleObject* item : thisArray) {
			// logger::info("Current item: {}", i);
			i += 1;
			if (item->createdItem == baseForm)  // if this recipe creates the same item we have here...
			{
				return item;
			}
		}
	} else {
		for (BGSConstructibleObject* item : thisArray) {
			// logger::info("Current item: {}", i);
			i += 1;
			if (item->createdItem == baseForm)  // if this recipe creates the same item we have here...
			{
				if (item->benchKeyword == temperKeyword)  // and the keyword is forge
				{
					return item;
				}
			}
		}
	}

	return nullptr;  //return none if this search failed
}

/*returns an array of all recipes that create this item and use the associated keyword.
If keyword string is empty string then it will ignore keywords and return all recipes.
*/
RE::BSTArray<RE::BGSConstructibleObject*> GetCraftingRecipeArray(RE::StaticFunctionTag*, RE::TESForm* baseForm,
	string keywordString)
{
	//   logger::info("running code");
	RE::TESForm* temperKeyword = TESForm::LookupByEditorID(keywordString);

	//holds all COBJs loaded.
	RE::BSTArray<RE::BGSConstructibleObject*> thisArray =
		RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSConstructibleObject>();

	//this array holds values that will be returned
	RE::BSTArray<RE::BGSConstructibleObject*> returnArray;  // = new RE::BSTArray<RE::BGSConstructibleObject*>;

	size_t len = keywordString.length();
	int i = 0;
	int j = 0;

	if (len == 0)  //if arg 2 is empty string
	{
		for (BGSConstructibleObject* item : thisArray) {
			i += 1;
			if (item->createdItem == baseForm)  // if this recipe creates the same item we have here...
			{
				j += 1;
				returnArray.resize(j);
				returnArray[j - 1] = item;
			}
		}
	} else  //otherwise its a keyword
	{
		for (BGSConstructibleObject* item : thisArray) {
			i += 1;
			if (item->createdItem == baseForm)  // if this recipe creates the same item we have here...
			{
				if (keywordString.length() == 0 || item->benchKeyword == temperKeyword)  // and the keyword is forge
				{
					//     logger::info("This is the recipe for {}", baseForm->GetName());
					j += 1;
					returnArray.resize(j);
					returnArray[j - 1] = item;
				}
			}
		}
	}

	return returnArray;
}

/*
Adds the keyword "kToAdd" to all armors that contain ALL of the keywords in array "kRequired"

returns the number of armors that were altered.

This is primarily useful for condition functions when you need to use 1 keyword to give more information than you normally can
since WornHasKeyword can only take 1 argument. You can use this function to consolidate sets of keywords into 1. 
for example 'ArmorHelmet' and 'ArmorHeavy' into 'HeavyHelmet'.
*/
int AddKeywordToAllArmorsWithKeywords(RE::StaticFunctionTag*, RE::BGSKeyword* kToAdd, RE::BSTArray<RE::BGSKeyword*> kRequired)
{
	int numAltered = 0;

	// holds all Objects of armor
	RE::BSTArray<RE::TESObjectARMO*> thisArray = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectARMO>();

	for (TESObjectARMO* item : thisArray) {
		int hasAllKeywords = 1;

		for (RE::BGSKeyword* kWord : kRequired) {
			if (!(item->HasKeyword(kWord))) {
				hasAllKeywords = 0;
				break;  // skip to next item if this one lacks the keyword
			}
		}

		if (hasAllKeywords == 1) {
			if (item->HasKeyword(kToAdd)) {
				;  // don't add the keyword if its already on it
			} else {
				item->AddKeyword(kToAdd);  //add keyword and increment counter of forms altered.
				string name = item->GetName();
				//   logger::info("{} has keyword", name);
				numAltered += 1;
			}
		}
	}
	return numAltered;
}

int AddKeywordToAllWeaponsWithKeywords(RE::StaticFunctionTag*, RE::BGSKeyword* kToAdd, RE::BSTArray<RE::BGSKeyword*> kRequired)
{
	int numAltered = 0;

	// holds all Objects of weapons
	RE::BSTArray<RE::TESObjectWEAP*> thisArray = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectWEAP>();

	for (TESObjectWEAP* item : thisArray) {
		int hasAllKeywords = 1;

		for (RE::BGSKeyword* kWord : kRequired) {
			if (!(item->HasKeyword(kWord))) {
				hasAllKeywords = 0;
				break;  // skip to next item if this one lacks the keyword
			}
		}

		if (hasAllKeywords == 1) {
			if (item->HasKeyword(kToAdd)) {
				;  // don't add the keyword if its already on it
			} else {
				item->AddKeyword(kToAdd);  // add keyword and increment counter of forms altered.
				string name = item->GetName();
				//   logger::info("{} has keyword", name);
				numAltered += 1;
			}
		}
	}
	return numAltered;
}

/*Returns an approximate amount of damage dealt If an actor were to hit another with a specific weapon 
(can be called from inside OnHit events). It takes into account perks that boost attack and weapon damage, 
but doesn't take into account critical hit chance or weapon tempering.

Thanks and credit go to Noah (Nobody) for explaining how Perk Entry Points work in detail.
*/
float GetActorExpectedDamage(RE::StaticFunctionTag*, RE::Actor* tActor, RE::TESObjectWEAP* tWeapon, RE::Actor* victim)
{
	float attackDamageMod = 1;
	float weaponDamageCalc =
		-1;  //this sets the exact damage of the weapon (base damage of sorts) before it is multiplied by above

	BGSEntryPoint::HandleEntryPoint(BGSPerkEntry::EntryPoint::kModAttackDamage, tActor, tWeapon, victim, &attackDamageMod);
	BGSEntryPoint::HandleEntryPoint(BGSPerkEntry::EntryPoint::kCalculateWeaponDamage, tActor, tWeapon, victim, &weaponDamageCalc);

	float totalDamage;
	if (weaponDamageCalc != -1)  // if the actor has any weapon damage calculating perks
	{
		totalDamage = weaponDamageCalc * attackDamageMod;
	} else  // otherwise we use weapon's base damage.
	{
		totalDamage = tWeapon->GetAttackDamage() * attackDamageMod;
	}

	//logger::info("{} dealt {} damage.", tActor->GetName(), totalDamage);
	return totalDamage;
}

/*
Sets Ammo forms as either playable or unplayable.
Unplayable ammunition cannot be viewed or dropped, but can still be equipped through a script.

Thanks and credit to Fenix31415 for help with setting up the project 
and understanding the stl::enumeration class.

*/
void SetAmmoPlayable(RE::StaticFunctionTag*, RE::TESAmmo* oAmmo, bool bPlayable)
{
	//logger::info("Name of Ammo is {}", oAmmo->GetName());
	if (bPlayable) {
	//	logger::info("flag before: {}", oAmmo->data.flags.underlying());
		oAmmo->data.flags.reset(RE::AMMO_DATA::Flag::kNonPlayable);  //kNonBolt
		//logger::info("setting ammo PLAYABLE");
		//logger::info("flag after: {}", oAmmo->data.flags.underlying());

	} else {
		//logger::info("flag before: {}", oAmmo->data.flags.underlying());
		oAmmo->data.flags.set(RE::AMMO_DATA::Flag::kNonPlayable);
		//logger::info("setting ammo UNPLAYABLE");
		//logger::info("flag after: {}", oAmmo->data.flags.underlying());
	}
	return;
}

/*
Sets a weapon's ImpactDataSet.
*/
void SetWeaponImpactDataSet(RE::StaticFunctionTag*, RE::TESObjectWEAP* tWeapon, RE::BGSImpactDataSet *dSet) 
{
	tWeapon->impactDataSet = dSet;
	//logger::info("set the impactdata set of {}", tWeapon->GetName());
}


/*
Returns the ImpactDataSet of a weapon.
*/
RE::BGSImpactDataSet* GetWeaponImpactDataSet(RE::StaticFunctionTag*, RE::TESObjectWEAP* tWeapon)
{
	//logger::info("GetWeaponImpactDataSet");
	return tWeapon->impactDataSet;
}

void SetExplosionDamage(RE::StaticFunctionTag*, RE::BGSExplosion *expl, float damage) 
{ 
	//logger::info("SetExplosionDamage");
	expl->data.damage = damage; 
}

float GetExplosionDamage(RE::StaticFunctionTag*, RE::BGSExplosion* expl)
{ 
	//logger::info("GetExplosionDamage");
	return expl->data.damage; 
}

void SetExplosionImpactDataSet(RE::StaticFunctionTag*, RE::BGSExplosion* expl, RE::BGSImpactDataSet* dSet)
{
	//logger::info("SetExplosionImpactDataSet");
	expl->data.impactDataSet = dSet;
}

RE::BGSImpactDataSet* GetExplosionImpactDataSet(RE::StaticFunctionTag*, RE::BGSExplosion* expl)
{ 
	//logger::info("GetExplosionImpactDataSet");
	return expl->data.impactDataSet;
}

/*
Sets the radius of the explosion in units
*/
void SetExplosionRadius(RE::StaticFunctionTag*, RE::BGSExplosion* expl, float radius)
{ 
	//logger::info("SetExplosionRadius");
	expl->data.radius = radius; 
}

/*
Returns the radius of an explosion in units
*/
float GetExplosionRadius(RE::StaticFunctionTag*, RE::BGSExplosion* expl)
{ 
	//logger::info("GetExplosionRadius");
	return expl->data.radius; 
}

/*
Sets the description of perk 'perkCopy' to the description of 'perkPaste'.
In other words, copies the description from perkCopy and pastes it onto perkPaste.
*/
void SetPerkDescription(RE::StaticFunctionTag*, RE::BGSPerk* perkPaste, RE::BGSPerk* perkCopy) {
	perkPaste->descriptionText = perkCopy->descriptionText;
	//logger::info("SetPerkDescription");
}

/*
Sets the perk as playable or nonplayable. Doesn't really do anything.
*/
void SetPerkPlayable(RE::StaticFunctionTag*, RE::BGSPerk* perk, bool bPlayable)
{ 
	//logger::info("SetPerkPlayable");
	perk->data.playable = bPlayable; 
}

/*
Sets the explosion of a projectile
*/
void SetProjectileExplosion(RE::StaticFunctionTag*, BGSProjectile* proj, RE::BGSExplosion* expl) {
	proj->data.explosionType = expl;
}

/*
Sets the sound level of a weapon. 
0 = silent, 1 = normal, 2 = loud, 3 = very loud
Does NOT persist across saves. You'll need a maintenance script for this.
*/
void SetWeaponDetectionLevel(RE::StaticFunctionTag*, RE::TESObjectWEAP* tWeapon, int sLevel)
{ 
	//logger::info("Setting detection level to {}", sLevel);
	switch (sLevel) {
	case 0:
		tWeapon->soundLevel = SOUND_LEVEL::kSilent; 
		break;
	case 1:
		tWeapon->soundLevel = SOUND_LEVEL::kNormal;
		break;
	case 2:
		tWeapon->soundLevel = SOUND_LEVEL::kLoud;
		break;
	case 3:
		tWeapon->soundLevel = SOUND_LEVEL::kVeryLoud;
		break;
	default:
		break;
	}
}

/*
Returns the detection level of a weapon.
0 = silent, 1 = normal, 2 = loud, 3 = very loud, -1 = error
*/
int GetWeaponDetectionLevel(RE::StaticFunctionTag*, RE::TESObjectWEAP* tWeapon)
{
	int toreturn = -1;
	logger::info("Getting detection level");
	if (tWeapon->soundLevel == SOUND_LEVEL::kSilent) {
		//logger::info("silent");
		toreturn = 0;
	} else if (tWeapon->soundLevel == SOUND_LEVEL::kNormal) {
		//logger::info("normal");
		toreturn = 1;
	} else if (tWeapon->soundLevel == SOUND_LEVEL::kLoud) {
		//logger::info("loud");
		toreturn = 2;
	} else if (tWeapon->soundLevel == SOUND_LEVEL::kVeryLoud) {
		//logger::info("very loud");
		toreturn = 3;
	} else {
		//logger::info("ERROR");
		toreturn = -1;  // return -1 if an error
	}
	return toreturn;
}

/*
Copies the conditions of perkCopy onto perkPaste.
These are the conditions to Select/Get the perk, not the Perk Entry Point Conditions.
*/
void SetPerkConditions(RE::StaticFunctionTag*, RE::BGSPerk* perkPaste, RE::BGSPerk* perkCopy)
{
	logger::info("Copying conditions from {} to {}", perkCopy->GetName(), perkPaste->GetName());
	perkPaste->perkConditions = perkCopy->perkConditions;
}

/*
Sets the enchantment of the explosion to ench
*/
void SetExplosionEnchantment(RE::StaticFunctionTag*, RE::BGSExplosion* expl, RE::EnchantmentItem *ench)
{ 
	//logger::info("Setting Enchantment");
	expl->formEnchanting = ench;
}

/*
Returns the enchantment attached to this explosion
*/
EnchantmentItem* GetExplosionEnchantment(RE::StaticFunctionTag*, RE::BGSExplosion* expl) { 
	//logger::info("Explosion is {}", expl->GetName());
	return expl->formEnchanting; 
}

bool BindPapyrusFunctions(RE::BSScript::IVirtualMachine* vm)
{
	vm->RegisterFunction("GetTemplateWeapon", "DennysPapyrusFunctions", GetTemplateWeapon);
	vm->RegisterFunction("GetTemplateArmor", "DennysPapyrusFunctions", GetTemplateArmor);
	vm->RegisterFunction("GetWeaponTemperingRecipe", "DennysPapyrusFunctions", GetWeaponTemperingRecipe);
	vm->RegisterFunction("GetArmorTemperingRecipe", "DennysPapyrusFunctions", GetArmorTemperingRecipe);
	vm->RegisterFunction("GetCraftingRecipe", "DennysPapyrusFunctions", GetCraftingRecipe);
	vm->RegisterFunction("GetCraftingRecipeArray", "DennysPapyrusFunctions", GetCraftingRecipeArray);
	vm->RegisterFunction("AddKeywordToAllArmorsWithKeywords", "DennysPapyrusFunctions", AddKeywordToAllArmorsWithKeywords);
	vm->RegisterFunction("AddKeywordToAllWeaponsWithKeywords", "DennysPapyrusFunctions", AddKeywordToAllWeaponsWithKeywords);
	
	vm->RegisterFunction("GetActorExpectedDamage", "DennysPapyrusFunctions", GetActorExpectedDamage);
	vm->RegisterFunction("SetAmmoPlayable", "DennysPapyrusFunctions", SetAmmoPlayable);
	vm->RegisterFunction("SetWeaponImpactDataSet", "DennysPapyrusFunctions", SetWeaponImpactDataSet);
	vm->RegisterFunction("GetWeaponImpactDataSet", "DennysPapyrusFunctions", GetWeaponImpactDataSet);
	vm->RegisterFunction("SetExplosionDamage", "DennysPapyrusFunctions", SetExplosionDamage);
	vm->RegisterFunction("GetExplosionDamage", "DennysPapyrusFunctions", GetExplosionDamage);
	vm->RegisterFunction("SetExplosionRadius", "DennysPapyrusFunctions", SetExplosionRadius);
	vm->RegisterFunction("GetExplosionRadius", "DennysPapyrusFunctions", GetExplosionRadius);
	vm->RegisterFunction("SetPerkDescription", "DennysPapyrusFunctions", SetPerkDescription);
	vm->RegisterFunction("SetPerkPlayable", "DennysPapyrusFunctions", SetPerkPlayable);

	vm->RegisterFunction("SetProjectileExplosion", "DennysPapyrusFunctions", SetProjectileExplosion);
	vm->RegisterFunction("SetWeaponDetectionLevel", "DennysPapyrusFunctions", SetWeaponDetectionLevel);
	vm->RegisterFunction("GetWeaponDetectionLevel", "DennysPapyrusFunctions", GetWeaponDetectionLevel);

	vm->RegisterFunction("SetPerkConditions", "DennysPapyrusFunctions", SetPerkConditions);
	vm->RegisterFunction("SetExplosionEnchantment", "DennysPapyrusFunctions", SetExplosionEnchantment);
	vm->RegisterFunction("GetExplosionEnchantment", "DennysPapyrusFunctions", GetExplosionEnchantment);
	
	// logger::info("successfully registered function");
	return true;
}

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
	switch (message->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		//
		
		break;
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	auto g_messaging = reinterpret_cast<SKSE::MessagingInterface*>(a_skse->QueryInterface(SKSE::LoadInterface::kMessaging));
	if (!g_messaging) {
		logger::critical("Failed to load messaging interface! This error is fatal, plugin will not load.");
		return false;
	}

	logger::info("loaded successfully");

	SKSE::Init(a_skse);
	SKSE::AllocTrampoline(1 << 10);
	SKSE::GetPapyrusInterface()->Register(BindPapyrusFunctions);

	g_messaging->RegisterListener("SKSE", SKSEMessageHandler);

	return true;
}
