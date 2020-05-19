// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "eos_common.h"

#pragma pack(push, 8)

EXTERN_C typedef struct EOS_ConnectHandle* EOS_HConnect;

/** Max length of an external account id in string form */
#define EOS_CONNECT_EXTERNAL_ACCOUNT_ID_MAX_LENGTH 256

/**
 * List of the supported identity providers to authenticate a user.
 *
 * The type of authentication token is specific to each provider.
 * Tokens in string format should be passed as-is to the function,
 * and tokens retrieved as raw byte arrays should be converted to string using
 * the EOS_ByteArray_ToString API before passing them to the Connect Interface API.
 *
 * @see EOS_Connect_Login
 * @see EOS_Connect_Credentials
 */
EOS_ENUM(EOS_EExternalCredentialType,
	/** 
	 * Epic Games User Token
	 *
	 * Acquired using EOS_Auth_CopyUserAuthToken that returns EOS_Auth_Token::AccessToken 
	 */
	EOS_ECT_EPIC = 0,
	/**
	 * Steam Encrypted App Ticket
	 *
	 * Generated using the ISteamUser::RequestEncryptedAppTicket API of Steamworks SDK.
	 * As the ticket generation parameters, use pDataToInclude(NULL) and cbDataToInclude(0).
	 *
	 * Use the EOS_ByteArray_ToString API to pass the retrieved ticket buffer as a hex-encoded string for the EOS_Connect_Login API.
	 */
	EOS_ECT_STEAM_APP_TICKET = 1,
	/**
	 * Playstation ID Token
	 *
	 * Retrieved from the Playstation SDK. Please see first-party documentation for additional information.
	 */
	EOS_ECT_PSN_ID_TOKEN = 2,
	/**
	 * Xbox Live User's XSTS Token
	 *
	 * Retrieved from the Xbox One XDK. Please see first-party documentation for additional information.
	 */
	EOS_ECT_XBL_XSTS_TOKEN = 3,
	/**
	 * Discord Access Token
	 */
	EOS_ECT_DISCORD_ACCESS_TOKEN = 4,
	/**
	 * GOG Galaxy Encrypted Session Ticket
	 */
	EOS_ECT_GOG_SESSION_TICKET = 5,
	/**
	 * Nintendo Account ID Token
	 *
	 * This token identifies the user's Nintendo account and is acquired using web flow authentication.
	 * On Nintendo Switch, the first time login requires the user to authenticate using their Nintendo account,
	 * after which the login is automatic and uses the primary authentication method with NSA ID Token.
	 */
	EOS_ECT_NINTENDO_ID_TOKEN = 6,
	/**
	 * Nintendo Service Account ID Token
	 *
	 * This is the device specific authentication token that is first used on login and will be linked with a Nintendo user account.
	 */
	EOS_ECT_NINTENDO_NSA_ID_TOKEN = 7,
	/**
	 * Uplay Access Token
	 */
	EOS_ECT_UPLAY_ACCESS_TOKEN = 8,
	/**
	 * OpenID Provider Access Token
	 */
	EOS_ECT_OPENID_ACCESS_TOKEN = 9,
	/**
	 * Device ID access token that identifies the current locally logged in user profile on the local device.
	 * The local user profile here refers to the operating system user login, for example the user's Windows Account
	 * or on a mobile device the default active user profile.
	 *
	 * This credential type is used to automatically login the local user using the EOS Connect Device ID feature.
	 *
	 * The intended use of the Device ID feature is to allow automatically logging in the user on a mobile device
	 * and to allow playing the game without requiring the user to necessarily login using a real user account at all.
	 * This makes a seamless first-time experience possible and allows linking the local device with a real external
	 * user account at a later time, sharing the same EOS_ProductUserId that is being used with the Device ID feature.
	 *
	 * @see EOS_Connect_CreateDeviceId
	 */
	EOS_ECT_DEVICEID_ACCESS_TOKEN = 10,
	/**
	 * Apple Identity Token
	 */
	EOS_ECT_APPLE_ID_TOKEN = 11
);

/** The most recent version of the EOS_Connect_Credentials struct. */
#define EOS_CONNECT_CREDENTIALS_API_LATEST 1

/**
 * A structure that contains external login credentials.
 * 
 * This is part of the input structure EOS_Connect_LoginOptions
 *
 * @see EOS_EExternalCredentialType
 * @see EOS_Connect_Login
 */ 
EOS_STRUCT(EOS_Connect_Credentials, (
	/** Version of the API */
	int32_t ApiVersion;
	/** External token associated with the user logging in */
	const char* Token;
	/** Type of external login. Needed to identify the auth method to use */
	EOS_EExternalCredentialType Type;
));

/** Max length of a display name, not including the terminating null */
#define EOS_CONNECT_USERLOGININFO_DISPLAYNAME_MAX_LENGTH 32

/** The most recent version of the EOS_Connect_UserLoginInfo struct. */
#define EOS_CONNECT_USERLOGININFO_API_LATEST 1

/**
 * Additional information about the local user.
 *
 * As the information passed here is client-controlled and not part of the user authentication tokens, it is only treated as non-authoritative informational data to be used by some of the feature services. For example displaying player names in Leaderboards rankings.
 */
EOS_STRUCT(EOS_Connect_UserLoginInfo, (
	/** Version of the API */
	int32_t ApiVersion;
	/** The user’s display name on the identity provider systems as UTF-8 encoded null-terminated string. The length of the name can be at maximum up to EOS_CONNECT_USERLOGININFO_DISPLAYNAME_MAX_LENGTH bytes. */
	const char* DisplayName;
));


/** The most recent version of the EOS_Connect_Login API. */
#define EOS_CONNECT_LOGIN_API_LATEST 2

/**
 * Input parameters for the EOS_Connect_Login Function.
 */
EOS_STRUCT(EOS_Connect_LoginOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Credentials specified for a given login method */
	const EOS_Connect_Credentials* Credentials;
	/**
	 * Additional non-authoritative information about the local user.
	 *
	 * This field is required to be set for user authentication with Apple and Nintendo, as well as with the Device ID feature login. When using other identity providers, set to NULL.
	 */
	const EOS_Connect_UserLoginInfo* UserLoginInfo;
));

/**
 * Output parameters for the EOS_Connect_Login Function.
 */
EOS_STRUCT(EOS_Connect_LoginCallbackInfo, (
	/** Result code for the operation. EOS_Success is returned for a successful query, otherwise one of the error codes is returned. See eos_result.h */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Connect_Login */
	void* ClientData;
	/** If login was succesful, this is the account ID of the local player that logged in */
	EOS_ProductUserId LocalUserId;
	/** 
	 * If the user was not found with credentials passed into EOS_Connect_Login, 
	 * this continuance token can be passed to either EOS_Connect_CreateUser 
	 * or EOS_Connect_LinkAccount to continue the flow
	 */
	EOS_ContinuanceToken ContinuanceToken;
));

/**
 * Function prototype definition for callbacks passed to EOS_Connect_Login
 * @param Data A EOS_Connect_LoginCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_Connect_OnLoginCallback, const EOS_Connect_LoginCallbackInfo* Data);

/** The most recent version of the EOS_Connect_CreateUser API. */
#define EOS_CONNECT_CREATEUSER_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_CreateUser Function.
 */
EOS_STRUCT(EOS_Connect_CreateUserOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Continuance token from previous call to EOS_Connect_Login */
	EOS_ContinuanceToken ContinuanceToken;
));

/**
 * Output parameters for the EOS_Connect_CreateUser Function.
 */
EOS_STRUCT(EOS_Connect_CreateUserCallbackInfo, (
	/** Result code for the operation. EOS_Success is returned for a successful query, otherwise one of the error codes is returned. See eos_result.h */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Connect_CreateUser */
	void* ClientData;
	/** Account ID of the local player created by this operation */
	EOS_ProductUserId LocalUserId;
));

EOS_DECLARE_CALLBACK(EOS_Connect_OnCreateUserCallback, const EOS_Connect_CreateUserCallbackInfo* Data);

/** The most recent version of the EOS_Connect_LinkAccount API. */
#define EOS_CONNECT_LINKACCOUNT_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_LinkAccount Function.
 */
EOS_STRUCT(EOS_Connect_LinkAccountOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Existing logged in user that will link to the external account referenced by the continuance token */
	EOS_ProductUserId LocalUserId;
	/** Continuance token from previous call to EOS_Connect_Login */
	EOS_ContinuanceToken ContinuanceToken;
));

/**
 * Output parameters for the EOS_Connect_LinkAccount Function.
 */
EOS_STRUCT(EOS_Connect_LinkAccountCallbackInfo, (
	/** Result code for the operation. EOS_Success is returned for a successful query, otherwise one of the error codes is returned. See eos_result.h */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Connect_LinkAccount */
	void* ClientData;
	/** Existing logged in user that had external auth linked */
	EOS_ProductUserId LocalUserId;
));

/**
 * Function prototype definition for callbacks passed to EOS_Connect_LinkAccount
 * @param Data A EOS_Connect_LinkAccountCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_Connect_OnLinkAccountCallback, const EOS_Connect_LinkAccountCallbackInfo* Data);

/** The most recent version of the EOS_Connect_CreateDeviceId API. */
#define EOS_CONNECT_CREATEDEVICEID_API_LATEST 1

/** Max length of a display name, not including the terminating null */
#define EOS_CONNECT_CREATEDEVICEID_DEVICEMODEL_MAX_LENGTH 64

/**
 * Input parameters for the EOS_Connect_CreateDeviceId Function.
 */
EOS_STRUCT(EOS_Connect_CreateDeviceIdOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/**
	 * A freeform text description identifying the device type and model,
	 * which can be used in account linking management to allow the player
	 * and customer support to identify different devices linked to an EOS
	 * user keychain. For example 'iPhone 6S' or 'PC Windows'.
	 *
	 * The input string must be in UTF-8 character format, with a maximum
	 * length of 64 characters. Longer string will be silently truncated.
	 *
	 * This field is required to be present.
	 */
	const char* DeviceModel;
));

/**
 * Output parameters for the EOS_Connect_CreateDeviceId Function.
 */
EOS_STRUCT(EOS_Connect_CreateDeviceIdCallbackInfo, (
	/** Result code for the operation. EOS_Success is returned for a successful query, otherwise one of the error codes is returned. See eos_result.h */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Connect_CreateDeviceId */
	void* ClientData;
));

EOS_DECLARE_CALLBACK(EOS_Connect_OnCreateDeviceIdCallback, const EOS_Connect_CreateDeviceIdCallbackInfo* Data);

/** The most recent version of the EOS_Connect_DeleteDeviceId API. */
#define EOS_CONNECT_DELETEDEVICEID_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_DeleteDeviceId Function.
 */
EOS_STRUCT(EOS_Connect_DeleteDeviceIdOptions, (
	/** Version of the API */
	int32_t ApiVersion;
));

/**
 * Output parameters for the EOS_Connect_DeleteDeviceId Function.
 */
EOS_STRUCT(EOS_Connect_DeleteDeviceIdCallbackInfo, (
	/** Result code for the operation. EOS_Success is returned for a successful query, otherwise one of the error codes is returned. See eos_result.h */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Connect_DeleteDeviceId */
	void* ClientData;
));

EOS_DECLARE_CALLBACK(EOS_Connect_OnDeleteDeviceIdCallback, const EOS_Connect_DeleteDeviceIdCallbackInfo* Data);

/** The most recent version of the EOS_Connect_QueryExternalAccountMappings API. */
#define EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_API_LATEST 1

/** Maximum number of account ids that can be queried at once */
#define EOS_CONNECT_QUERYEXTERNALACCOUNTMAPPINGS_MAX_ACCOUNT_IDS 128

/**
 * Input parameters for the EOS_Connect_QueryExternalAccountMappings Function.
 */
EOS_STRUCT(EOS_Connect_QueryExternalAccountMappingsOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Existing logged in user that is querying account mappings */
	EOS_ProductUserId LocalUserId;
	/** External auth service supplying the account ids in string form */
	EOS_EExternalAccountType AccountIdType;
	/** An array of external account ids to map to the product user id representation */
	const char** ExternalAccountIds;
	/** Number of account ids to query */
	uint32_t ExternalAccountIdCount;
));

/**
 * Output parameters for the EOS_Connect_QueryExternalAccountMappings Function.
 */
EOS_STRUCT(EOS_Connect_QueryExternalAccountMappingsCallbackInfo, (
	/** Result code for the operation. EOS_Success is returned for a successful query, otherwise one of the error codes is returned. See eos_result.h */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Connect_QueryExternalAccountMappings */
	void* ClientData;
	/** Existing logged in user that made the request */
	EOS_ProductUserId LocalUserId;
));

/**
 * Function prototype definition for callbacks passed to EOS_Connect_QueryExternalAccountMappings
 * @param Data A EOS_Connect_QueryExternalAccountMappingsCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_Connect_OnQueryExternalAccountMappingsCallback, const EOS_Connect_QueryExternalAccountMappingsCallbackInfo* Data);

/** The most recent version of the EOS_Connect_GetExternalAccountMappings API. */
#define EOS_CONNECT_GETEXTERNALACCOUNTMAPPINGS_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_GetExternalAccountMappings Function.
 */
EOS_STRUCT(EOS_Connect_GetExternalAccountMappingsOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Existing logged in user that is querying account mappings */
	EOS_ProductUserId LocalUserId;
	/** External auth service supplying the account ids in string form */
	EOS_EExternalAccountType AccountIdType;
	/** Target user to retrieve the mapping for, as an external account id */
	const char* TargetExternalUserId;
));

/** The most recent version of the EOS_Connect_QueryProductUserIdMappings API. */
#define EOS_CONNECT_QUERYPRODUCTUSERIDMAPPINGS_API_LATEST 2

/**
 * Input parameters for the EOS_Connect_QueryProductUserIdMappings Function.
 */
EOS_STRUCT(EOS_Connect_QueryProductUserIdMappingsOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Existing logged in user that is querying account mappings */
	EOS_ProductUserId LocalUserId;
	/** Deprecated - all external mappings are included in this call, it is no longer necessary to specify this value */
	EOS_EExternalAccountType AccountIdType_DEPRECATED;
	/** An array of product user ids to query for the given external account representation */
	EOS_ProductUserId* ProductUserIds;
	/** Number of account ids to query */
	uint32_t ProductUserIdCount;
));

/**
 * Output parameters for the EOS_Connect_QueryProductUserIdMappings Function.
 */
EOS_STRUCT(EOS_Connect_QueryProductUserIdMappingsCallbackInfo, (
	/** Result code for the operation. EOS_Success is returned for a successful query, otherwise one of the error codes is returned. See eos_result.h */
	EOS_EResult ResultCode;
	/** Context that was passed into EOS_Connect_QueryProductUserIdMappings */
	void* ClientData;
	/** Existing logged in user that made the request */
	EOS_ProductUserId LocalUserId;
));

/**
 * Function prototype definition for callbacks passed to EOS_Connect_QueryProductUserIdMappings
 * @param Data A EOS_Connect_QueryProductUserIdMappingsCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_Connect_OnQueryProductUserIdMappingsCallback, const EOS_Connect_QueryProductUserIdMappingsCallbackInfo* Data);

/** The most recent version of the EOS_Connect_GetProductUserIdMapping API. */
#define EOS_CONNECT_GETPRODUCTUSERIDMAPPING_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_GetProductUserIdMapping Function.
 */
EOS_STRUCT(EOS_Connect_GetProductUserIdMappingOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Existing logged in user that is querying account mappings */
	EOS_ProductUserId LocalUserId;
	/** External auth service mapping to retrieve */
	EOS_EExternalAccountType AccountIdType;
	/** Target product user id to retrieve */
	EOS_ProductUserId TargetProductUserId;
));

/** The most recent version of the EOS_Connect_GetProductUserExternalAccountCount API. */
#define EOS_CONNECT_GETPRODUCTUSEREXTERNALACCOUNTCOUNT_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_GetProductUserExternalAccountCount Function.
 */
EOS_STRUCT(EOS_Connect_GetProductUserExternalAccountCountOptions, (
	/** Version of the API */
	int32_t ApiVersion;
	/** Product user id to look for when getting external account info count from the cache. */
	EOS_ProductUserId TargetUserId;
));

/** The most recent version of the EOS_Connect_CopyProductUserExternalAccountByIndex API. */
#define EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYINDEX_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_CopyProductUserExternalAccountByIndex Function.
 */
EOS_STRUCT(EOS_Connect_CopyProductUserExternalAccountByIndexOptions, (
	/** API Version of the EOS_Connect_CopyProductUserExternalAccountByIndex function. */
	int32_t ApiVersion;
	/** Product user id to look for when copying external account info from the cache. */
	EOS_ProductUserId TargetUserId;
	/** Index of the external account info to retrieve from the cache. */
	uint32_t ExternalAccountInfoIndex;
));

/** The most recent version of the EOS_Connect_CopyProductUserExternalAccountByAccountType API. */
#define EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYACCOUNTTYPE_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_CopyProductUserExternalAccountByAccountType Function.
 */
EOS_STRUCT(EOS_Connect_CopyProductUserExternalAccountByAccountTypeOptions, (
	/** API Version of the EOS_Connect_CopyProductUserExternalAccountByAccountType function. */
	int32_t ApiVersion;
	/** Product user id to look for when copying external account info from the cache. */
	EOS_ProductUserId TargetUserId;
	/** External auth service account type to look for when copying external account info from the cache. */
	EOS_EExternalAccountType AccountIdType;
));

/** The most recent version of the EOS_Connect_CopyProductUserExternalAccountByAccountId API. */
#define EOS_CONNECT_COPYPRODUCTUSEREXTERNALACCOUNTBYACCOUNTID_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_CopyProductUserExternalAccountByAccountId Function.
 */
EOS_STRUCT(EOS_Connect_CopyProductUserExternalAccountByAccountIdOptions, (
	/** API Version of the EOS_Connect_CopyProductUserExternalAccountByAccountId function. */
	int32_t ApiVersion;
	/** Product user id to look for when copying external account info from the cache. */
	EOS_ProductUserId TargetUserId;
	/** External auth service account id to look for when copying external account info from the cache. */
	const char* AccountId;
));

/** The most recent version of the EOS_Connect_CopyProductUserInfo API. */
#define EOS_CONNECT_COPYPRODUCTUSERINFO_API_LATEST 1

/**
 * Input parameters for the EOS_Connect_CopyProductUserInfo Function.
 */
EOS_STRUCT(EOS_Connect_CopyProductUserInfoOptions, (
	/** API Version of the EOS_Connect_CopyProductUserInfo function. */
	int32_t ApiVersion;
	/** Product user id to look for when copying external account info from the cache. */
	EOS_ProductUserId TargetUserId;
));

/** Timestamp value representing an undefined time for last login time. */
#define EOS_CONNECT_TIME_UNDEFINED -1

/** The most recent version of the EOS_Connect_ExternalAccountInfo struct. */
#define EOS_CONNECT_EXTERNALACCOUNTINFO_API_LATEST 1

/**
 * Contains information about an external account info
 */
EOS_STRUCT(EOS_Connect_ExternalAccountInfo, (
	/** Version of the API. */
	int32_t ApiVersion;
	/** Product user id of the target user. */
	EOS_ProductUserId ProductUserId;
	/** Display name, can be null if not set. */
	const char* DisplayName;
	/** External account id. */
	const char* AccountId;
	/** The identity provider that owns the external account. */
	EOS_EExternalAccountType AccountIdType;
	/** The POSIX timestamp for the time the user last logged in, or EOS_CONNECT_TIME_UNDEFINED. */
	int64_t LastLoginTime;
));

/**
 * Release the memory associated with an external account info. This must be called on data retrieved from
 * EOS_Connect_CopyProductUserExternalAccountByIndex, EOS_Connect_CopyProductUserExternalAccountByAccountType,
 * EOS_Connect_CopyProductUserExternalAccountByAccountId or EOS_Connect_CopyProductUserInfo.
 *
 * @param ExternalAccountInfo The external account info data to release.
 *
 * @see EOS_Connect_CopyProductUserExternalAccountByIndex
 * @see EOS_Connect_CopyProductUserExternalAccountByAccountType
 * @see EOS_Connect_CopyProductUserExternalAccountByAccountId
 * @see EOS_Connect_CopyProductUserInfo
 */
EOS_DECLARE_FUNC(void) EOS_Connect_ExternalAccountInfo_Release(EOS_Connect_ExternalAccountInfo* ExternalAccountInfo);

/** The most recent version of the EOS_Connect_AddNotifyAuthExpiration API. */
#define EOS_CONNECT_ADDNOTIFYAUTHEXPIRATION_API_LATEST 1
/**
 * Structure containing information for the auth expiration notification callback
 */
EOS_STRUCT(EOS_Connect_AddNotifyAuthExpirationOptions, (
	/** API Version of the EOS_Connect_AddNotifyAuthExpirationOptions structure */
	int32_t ApiVersion;
));

/** The most recent version of the EOS_Connect_OnAuthExpirationCallback API. */
#define EOS_CONNECT_ONAUTHEXPIRATIONCALLBACK_API_LATEST 1

/**
 * Output parameters for the EOS_Connect_OnAuthExpirationCallback Function.
 */
EOS_STRUCT(EOS_Connect_AuthExpirationCallbackInfo, (
	/** Context that was passed into EOS_Connect_AddNotifyAuthExpiration */
	void* ClientData;
	/** Account ID of the local player whose status has changed */
	EOS_ProductUserId LocalUserId;
));

/**
 * Function prototype definition for notifications that come from EOS_Connect_AddNotifyAuthExpiration
 *
 * @param Data A EOS_Connect_AuthExpirationCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_Connect_OnAuthExpirationCallback, const EOS_Connect_AuthExpirationCallbackInfo* Data);


/** The most recent version of the EOS_Connect_AddNotifyLoginStatusChangedOptions API. */
#define EOS_CONNECT_ADDNOTIFYLOGINSTATUSCHANGED_API_LATEST 1
/**
 * Structure containing information or the connect user login status change callback
 */
EOS_STRUCT(EOS_Connect_AddNotifyLoginStatusChangedOptions, (
	/** API Version of the EOS_Connect_AddNotifyLoginStatusChangedOptions structure */
	int32_t ApiVersion;
));

/**
 * Output parameters for the EOS_Connect_OnLoginStatusChangedCallback Function.
 */
EOS_STRUCT(EOS_Connect_LoginStatusChangedCallbackInfo, (
	/** Context that was passed into EOS_Connect_AddNotifyLoginStatusChanged */
	void* ClientData;
	/** Account ID of the local player whose status has changed */
	EOS_ProductUserId LocalUserId;
	/** The status prior to the change */
	EOS_ELoginStatus PreviousStatus;
	/** The status at the time of the notification */
	EOS_ELoginStatus CurrentStatus;
));

/**
 * Function prototype definition for notifications that come from EOS_Connect_AddNotifyLoginStatusChanged
 *
 * @param Data A EOS_Connect_LoginStatusChangedCallbackInfo containing the output information and result
 */
EOS_DECLARE_CALLBACK(EOS_Connect_OnLoginStatusChangedCallback, const EOS_Connect_LoginStatusChangedCallbackInfo* Data);

#pragma pack(pop)
