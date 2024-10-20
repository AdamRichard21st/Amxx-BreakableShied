/*====================================================================================================
		AMX Mod X, based on AMX Mod by Aleksander Naszko ("OLO").
		Copyright (C) The AMX Mod X Development Team.
		
		This software is licensed under the GNU General Public License, version 3 or higher.
		Additional exceptions apply. For full license details, see LICENSE.txt or visit:
		https://alliedmods.net/amxmodx-license
====================================================================================================*/

#if defined		__custom_entities_data_inc__
	#endinput
#endif
#define		__custom_entities_data_inc__


/*
====================================================================================================
custom_entities_data.inc
	custom entities-data manipulation functions/stocks
	
these functions can be used to manipulate custom-data for entities (even players! [ANY VALID ENTITY]) easily, without worrying about
use MOD specific internal-vars (a.k.a pevs like iuser1/iuser2/fuser1/buser1......etc).

(example) you can just do:
	CED_SetCell(entity, "entity-type", 2);
	
and then:
	new entity_type;
	CED_GetCell(entity, "entity-type", entity_type)
	if (entity_type == 2)
	{
		data-field 'entity-type' is 2......do whatever you want
	}

so, you will never need use pev-vars for store data (since there is a small number of pevs to use, and many are used by the game mostly of time)
you can also update custom-data, just with setting it again (only if entites custom-data has not been freeded [CED_Free])

examples:

	setting values:
		cell-types:
			CED_SetCell(entity, "type", 5) // int value
			CED_SetCell(entity, "RespawnTime", 20.0) // float value
			CED_SetCell(entity, "CanRespawn", true) // bool value
	
		string-types:
		
			new unique_name[15];
			formatex(unique_name_var, 24, "uniquename");
			CED_SetString(entity, "var_name", unique_name)
			
			CED_SetString(entity, "constant_name", "a constant entity name")
	
		array-types:
			CED_SetArray(entity, "valid_owners", { 2, 4, 5 })
			
			new temp_data[3] = { 23, 51, 2 }
			CED_SetArray(entity, "temp_data", temp_data);
	
	getting values:
		cell-types:
			new type;
			CED_GetCell(entity, "type", type)
	
		string-types:
			new unique_name[15];
			CED_GetString(entity, "var_name", unique_name)
		
		array-types:
			new Array[3];
			CED_GetArray(entity, "valid_owners", Array)
		
	Updating a value:
		CED_SetCell(entity, "type", 25)
		CED_GetCell(entity, "type", type) // you will get '25'[NEW] instead of '5'[OLD]
		
		the same applies for string/array data-types
	
====================================================================================================
*/

#define		SEND_LOGS		true	// specifies if custom logs will be send (for error/debug info)

// entities custom-data types
enum _:ENTITIES_CUSTOM_DATA_TYPES
{
	CUSTOM_ENT_DATA_TYPE_CELL, // int/float/bool
	CUSTOM_ENT_DATA_TYPE_STRING,
	CUSTOM_ENT_DATA_TYPE_ARRAY
};

// acts like a unique-name for a entity (so data can be set/get easily)
// IF YOU DON'T KNOW WHAT ARE YOU DOING, CONSIDER DON'T TOUCH THIS
static const CUSTOM_DATA_ENTITIES_KEY[] = "ent_%d_custom_data";

// contains a complete list of entities that have custom-data stored
// KEEP-IN-MIND: YOU NEED TO FREE (CED_Free) THIS ON YOUR PLUGIN (plugin_end for example, or whatever you want)
static Trie:g_tCUSTOM_DATA_ENTITIES_LIST;

// contains a complete list of IDs of entities that have custom-data stored
static Array:g_aCUSTOM_DATA_ENTITIES_IDS_LIST;

// contains a complete list of data-field that a specific entity have stored
// KEEP-IN-MIND: YOU NEED TO FREE (CED_Free) THIS ON YOUR PLUGIN (plugin_end for example, or whatever you want)
static Trie:g_tENTITY_CUSTOM_DATA_LIST;

/*
==================================================
CED_Clear
	free all entity's custom-data stored
	
==================================================
*/

stock CED_Free()
{
	if (g_tCUSTOM_DATA_ENTITIES_LIST == Invalid_Trie)
		return;
	
	if (g_aCUSTOM_DATA_ENTITIES_IDS_LIST == Invalid_Array)
		return;
	
	if (g_tCUSTOM_DATA_ENTITIES_LIST == Invalid_Trie)
		return;
	
	new szKey[25];
	
	new iMaxEntities = ArraySize(g_aCUSTOM_DATA_ENTITIES_IDS_LIST);
	new iEntityId;
	for (new iEntity; iEntity < iMaxEntities; iEntity++)
	{
		iEntityId = ArrayGetCell(g_aCUSTOM_DATA_ENTITIES_IDS_LIST, iEntity);
		formatex(szKey, charsmax(szKey), CUSTOM_DATA_ENTITIES_KEY, iEntityId);
		
		TrieGetCell(g_tCUSTOM_DATA_ENTITIES_LIST, szKey, g_tENTITY_CUSTOM_DATA_LIST);
		TrieDestroy(g_tENTITY_CUSTOM_DATA_LIST);
	}
	TrieDestroy(g_tCUSTOM_DATA_ENTITIES_LIST);
	ArrayDestroy(g_aCUSTOM_DATA_ENTITIES_IDS_LIST);
}

/*
==================================================
CED_Clear
	clear all entity's custom-data stored

params:
	iEntity: entity-id

return:
	true: 	data has been cleared
	false: 	entity-id is invalid
==================================================
*/

stock bool:CED_Clear(const iEntity)
	return CED_SetEntCustomData(iEntity, _, "_CLEAR_DATA_");
	
/*
==================================================
CED_GetCell
	get (load) a CELL data-type (int/float/bool) for a entity

params:
	iEntity: 	entity-id
	szData: 	data-field where be loaded
	Value:		where data will be copied
	
return:
	true:		data has been loaded
	false:		either:
					entity-id is invalid
					entity doesn't have custom-data stored
					entity doesn't have specific custom-data (szData) stored
					data-type is invalid
==================================================
*/

stock bool:CED_GetCell(const iEntity, const szData[], &any:Value)
{
	new tempValue[2];
	new bool:bReturn = CED_GetEntCustomData(iEntity, CUSTOM_ENT_DATA_TYPE_CELL, szData, tempValue)
	Value = tempValue[0];
	return bReturn;
}

/*
==================================================
CED_SetCell
	set/update (store) a CELL data-type for a entity

params:
	iEntity: 	entity-id
	szData: 	data-field where be stored
	Value: 		data to store (CELL type: could be int/float/bool) 
	
return:
	true: 	data has been stored
	false: 	entity-id is invalid
==================================================
*/

stock bool:CED_SetCell(const iEntity, const szData[], const any:Value)
	return CED_SetEntCustomData(iEntity, CUSTOM_ENT_DATA_TYPE_CELL, szData, Value);

/*
==================================================
CED_GetString
	get (load) a STRING data-type for a entity

params:
	iEntity: 					entity-id
	szData: 					data-field where be loaded
	szValue:					buffer where string be copied
	(optional) iValueLength:	string-size will be copied (default is max-size [sizeof])
								KEEP-IN-MIND!: if you pass a custom-size, you may add to the size the zero-terminated character (a.k.a EOS)
								sice during load, this is removed (IF NOT, YOU MAYBE CAN LOST DATA!!)
								
								this means, that in place of doing this
								new szString[15];
								CED_GetString(entity, "strig_field", szString, charsmax(szString));
								
								you should do this:
								new szString[15];
								CED_GetString(entity, "strig_field", szString, szString);
	
return:
	true:						data has been loaded
	false:						either:
								entity-id is invalid
								entity doesn't have custom-data stored
								entity doesn't have specific custom-data (szData) stored
								data-type is invalid
==================================================
*/

stock bool:CED_GetString(const iEntity, const szData[], szValue[], const iValueLength = sizeof szValue)
	return CED_GetEntCustomData(iEntity, CUSTOM_ENT_DATA_TYPE_STRING, szData, szValue, iValueLength);

/*
==================================================
CED_SetString
	set/update (store) a STRING data-type for a entity

params:
	iEntity: 					entity-id
	szData: 					data-field where be stored
	szValue: 					data to store
	(not used)iValueLength: 	string size will be stored (unused; only for ARRAY data-types, see CED_SetArray)
	
return:	
	true: 	data has been stored
	false: 	entity-id is invalid
==================================================
*/

stock bool:CED_SetString(const iEntity, const szData[], const szValue[], const iValueLength = sizeof szValue)
	return CED_SetEntCustomData(iEntity, CUSTOM_ENT_DATA_TYPE_STRING, szData, _, szValue, iValueLength);

/*
==================================================
CED_GetArray
	get (load) a ARRAY data-type for a entity

params:
	iEntity: 					entity-id
	szData: 					data-field where be loaded
	Value:						buffer where array will be copied
	(optional) iValueLength:	array-size will be copied (default is max-size [sizeof])
	
return:
	true:						data has been loaded
	false:						either:
								entity-id is invalid
								entity doesn't have custom-data stored
								entity doesn't have specific custom-data (szData) stored
								data-type is invalid
==================================================
*/

stock bool:CED_GetArray(const iEntity, const szData[], Value[], const iValueLength = sizeof Value)
	return CED_GetEntCustomData(iEntity, CUSTOM_ENT_DATA_TYPE_ARRAY, szData, Value, iValueLength);

/*
==================================================
CED_SetArray
	set/update (store) a ARRAY data-type for a entity

params:
	iEntity: 					entity-id
	szData: 					data-field where be stored
	Value: 						data to store
	(optional)iValueLength: 	unlike CED_SetString, here this specifies the array-size to store (default is max-size [sizeof])
								most of times, this is not needed; the function itself calculates array-size on call
								but, if you want add a specific size, do at your own risk!
								
return:
	true: 	data has been stored
	false: 	entity-id is invalid
==================================================
*/

stock bool:CED_SetArray(const iEntity, const szData[], const Value[], const iValueLength = sizeof Value)
	return CED_SetEntCustomData(iEntity, CUSTOM_ENT_DATA_TYPE_ARRAY, szData, _, Value, iValueLength);

/*
==================================================
CED_GetEntCustomData
	get (load) a custom-data-type for a specific entity
	AVOID USING THIS, AND INSTEAD USE THE ONES FOR DATA-SPECIFIC TYPES (CED_Get[Cell/String/Array])

params:
	iEntity: 				entity-id
	iDataType: 				data-type where be stored
	szData: 				data-field to store
	StringOrArrayValue:		buffer where data will be copied (for [STRING/ARRAY] data-types)
	iStringOrArrayLength: 	data size will be copied (for [STRING/ARRAY] data-types)
	
return:
	true:						data has been loaded
	false:						either:
								entity-id is invalid
								entity doesn't have custom-data stored
								entity doesn't have specific custom-data (szData) stored
								data-type is invalid
								
								
	if iDataType == CUSTOM_ENT_DATA_TYPE_CELL
	iStringOrArrayLength: 		whill have the data
	
	if iDataType = CUSTOM_ENT_DATA_TYPE_STRING or CUSTOM_ENT_DATA_TYPE_ARRAY
	StringOrArrayValue:			whill have the data
==================================================
*/

stock bool:CED_GetEntCustomData(const iEntity, const iDataType = -1, const szData[], StringOrArrayValue[] = 0, iStringOrArrayLength = 0)
{
	if (g_tCUSTOM_DATA_ENTITIES_LIST == Invalid_Trie)
		return false;
	
	if (g_aCUSTOM_DATA_ENTITIES_IDS_LIST == Invalid_Array)
		return false;
	
	if (g_tCUSTOM_DATA_ENTITIES_LIST == Invalid_Trie)
		return false;
	
	if (!is_valid_ent(iEntity))
	{
		#if defined		SEND_LOGS
		{
			#if		SEND_LOGS	==	true
			{
				log_amx("GetEntCustomData(): Invalid entity-id (%d).", iEntity);
			}
			#endif
		}
		#endif
		return false;
	}
	
	static szKey[25];
	formatex(szKey, charsmax(szKey), CUSTOM_DATA_ENTITIES_KEY, iEntity);
	if (!TrieKeyExists(g_tCUSTOM_DATA_ENTITIES_LIST, szKey))
	{
		#if defined		SEND_LOGS
		{
			#if		SEND_LOGS	==	true
			{
				log_amx("GetEntCustomData(): entity '%d' doesn't have custom-data.", iEntity);
			}
			#endif
		}
		#endif
		return false;
	}
	
	TrieGetCell(g_tCUSTOM_DATA_ENTITIES_LIST, szKey, g_tENTITY_CUSTOM_DATA_LIST);
	if (!TrieKeyExists(g_tENTITY_CUSTOM_DATA_LIST, szData))
	{
		#if defined		SEND_LOGS
		{
			#if		SEND_LOGS	==	true
			{
				log_amx("GetEntCustomData(): entity '%d' doesn't have stored custom-data for '%s'.", iEntity, szData);
			}
			#endif
		}
		#endif
		return false;
	}
	
	switch (iDataType)
	{
		case CUSTOM_ENT_DATA_TYPE_CELL:
		{
			TrieGetCell(g_tENTITY_CUSTOM_DATA_LIST, szData, StringOrArrayValue[0]);
			return true;
		}
		case CUSTOM_ENT_DATA_TYPE_STRING:
		{
			TrieGetString(g_tENTITY_CUSTOM_DATA_LIST, szData, StringOrArrayValue, iStringOrArrayLength - 1);
			return true;
		}
		case CUSTOM_ENT_DATA_TYPE_ARRAY:
		{
			TrieGetArray(g_tENTITY_CUSTOM_DATA_LIST, szData, StringOrArrayValue, iStringOrArrayLength);
			return true;
		}
		default:
		{
			#if defined		SEND_LOGS
			{
				#if		SEND_LOGS	==	true
				{
					log_amx("GetEntCustomData(): Invalid custom-data type (%d).", iDataType);
				}
				#endif
			}
			#endif
			return false;
		}
	}
	return false;
}

/*
==================================================
CED_SetEntCustomData
	set (store) a custom-data-type for a specific entity
	AVOID USING THIS, AND INSTEAD USE THE ONES FOR DATA-SPECIFIC TYPES (CED_Set[Cell/String/Array])

params:
	iEntity: 				entity-id
	iDataType: 				data-type will be stored
	szData: 				data-field where be stored
	CellValue: 				data to store for CELL data-type ('iDataType' == CUSTOM_ENT_DATA_TYPE_CELL)
	StringOrArrayValue: 	data will be stored ('iDataType' == [CUSTOM_ENT_DATA_TYPE_STRING || CUSTOM_ENT_DATA_TYPE_ARRAY])
	iStringOrArrayLength: 	for ARRAY data-types, specifies the array size will be stored
	
return:
	true:		data has been stored
	false:		entity-id or data-type is invalid
==================================================
*/

stock bool:CED_SetEntCustomData(const iEntity, const iDataType = -1, const szData[], const any:CellValue = 0, const any:StringOrArrayValue[] = 0, const iStringOrArrayLength = 0)
{
	if (!is_valid_ent(iEntity))
	{
		#if defined		SEND_LOGS
		{
			#if		SEND_LOGS	==	true
			{
				log_amx("SetEntCustomData(): Invalid entity-id (%d).", iEntity);
			}
			#endif
		}
		#endif
		return false;
	}
	
	if (!g_tCUSTOM_DATA_ENTITIES_LIST)
		g_tCUSTOM_DATA_ENTITIES_LIST = TrieCreate();
	
	if (!g_aCUSTOM_DATA_ENTITIES_IDS_LIST)
		g_aCUSTOM_DATA_ENTITIES_IDS_LIST = ArrayCreate();
	
	static szKey[25];
	formatex(szKey, charsmax(szKey), CUSTOM_DATA_ENTITIES_KEY, iEntity);
	
	new bool:bEntHasCustomData = TrieKeyExists(g_tCUSTOM_DATA_ENTITIES_LIST, szKey)
	if (bEntHasCustomData)
		TrieGetCell(g_tCUSTOM_DATA_ENTITIES_LIST, szKey, g_tENTITY_CUSTOM_DATA_LIST);
	else
		g_tENTITY_CUSTOM_DATA_LIST = TrieCreate();
	
	if (equal(szData, "_CLEAR_DATA_") && g_tENTITY_CUSTOM_DATA_LIST > Invalid_Trie)
		TrieClear(g_tENTITY_CUSTOM_DATA_LIST);
	else
	{
		switch (iDataType)
		{
			case CUSTOM_ENT_DATA_TYPE_CELL: TrieSetCell(g_tENTITY_CUSTOM_DATA_LIST, szData, CellValue);
			case CUSTOM_ENT_DATA_TYPE_STRING: TrieSetString(g_tENTITY_CUSTOM_DATA_LIST, szData, StringOrArrayValue);
			case CUSTOM_ENT_DATA_TYPE_ARRAY: TrieSetArray(g_tENTITY_CUSTOM_DATA_LIST, szData, StringOrArrayValue, iStringOrArrayLength);
			default:
			{
				#if defined		SEND_LOGS
				{
					#if		SEND_LOGS	==	true
					{
						log_amx("SetEntCustomData(): Invalid custom-data type (%d).", iDataType);
					}
					#endif
				}
				#endif
				return false;
			}
				
		}
	}
	
	if (!bEntHasCustomData)
		ArrayPushCell(g_aCUSTOM_DATA_ENTITIES_IDS_LIST, iEntity);
	
	TrieSetCell(g_tCUSTOM_DATA_ENTITIES_LIST, szKey, g_tENTITY_CUSTOM_DATA_LIST);
	return true;
}