module;

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

export module Extern.GameNetworkingSockets;

export namespace GNS
{
const auto a = k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged;

using SteamNetConnectionStatusChangedCallback_t = SteamNetConnectionStatusChangedCallback_t;
using HSteamNetConnection = HSteamNetConnection;
using ISteamNetworkingSockets = ISteamNetworkingSockets;
using HSteamNetPollGroup = HSteamNetPollGroup;
using HSteamListenSocket = HSteamListenSocket;
using SteamNetworkingIPAddr = SteamNetworkingIPAddr;
using SteamNetworkingConfigValue_t = SteamNetworkingConfigValue_t;
using ISteamNetworkingMessage = ISteamNetworkingMessage;
using SteamDatagramErrMsg = SteamDatagramErrMsg;
using SteamNetworkingMicroseconds = SteamNetworkingMicroseconds;
using ESteamNetworkingSocketsDebugOutputType = ESteamNetworkingSocketsDebugOutputType;

constexpr auto GameNetworkingSockets = SteamNetworkingSockets;
constexpr auto GameNetworkingUtils = SteamNetworkingUtils;
constexpr auto GameNetworkingSocketsInit = GameNetworkingSockets_Init;
constexpr auto GameNetworkingSocketsKill = GameNetworkingSockets_Kill;

namespace HSteamNetPollGroup_Enum
{
constexpr auto Invalid = k_HSteamNetPollGroup_Invalid;

}

namespace HSteamListenSocket_Enum
{
constexpr auto Invalid = k_HSteamListenSocket_Invalid;
}

namespace nSteamNetworkingSend
{
constexpr auto Reliable = k_nSteamNetworkingSend_Reliable;
constexpr auto Unreliable = k_nSteamNetworkingSend_Unreliable;
} // namespace nSteamNetworkingSend

namespace ESteamNetworkingSocketsDebugOutputType_Enum
{
constexpr auto Msg = k_ESteamNetworkingSocketsDebugOutputType_Msg;
constexpr auto Bug = k_ESteamNetworkingSocketsDebugOutputType_Bug;
} // namespace ESteamNetworkingSocketsDebugOutputType_Enum

namespace ESteamNetworkingConnectionState
{
constexpr auto None = k_ESteamNetworkingConnectionState_None;
constexpr auto Connecting = k_ESteamNetworkingConnectionState_Connecting;
constexpr auto FindingRoute = k_ESteamNetworkingConnectionState_FindingRoute;
constexpr auto Connected = k_ESteamNetworkingConnectionState_Connected;
constexpr auto ClosedByPeer = k_ESteamNetworkingConnectionState_ClosedByPeer;
constexpr auto ProblemDetectedLocally = k_ESteamNetworkingConnectionState_ProblemDetectedLocally;
constexpr auto FinWait = k_ESteamNetworkingConnectionState_FinWait;
constexpr auto Linger = k_ESteamNetworkingConnectionState_Linger;
constexpr auto Dead = k_ESteamNetworkingConnectionState_Dead;
constexpr auto _Force32Bit = k_ESteamNetworkingConnectionState__Force32Bit;
} // namespace ESteamNetworkingConnectionState

namespace ESteamNetworkingConfig
{
constexpr auto Invalid = k_ESteamNetworkingConfig_Invalid;
constexpr auto TimeoutInitial = k_ESteamNetworkingConfig_TimeoutInitial;
constexpr auto TimeoutConnected = k_ESteamNetworkingConfig_TimeoutConnected;
constexpr auto SendBufferSize = k_ESteamNetworkingConfig_SendBufferSize;
constexpr auto ConnectionUserData = k_ESteamNetworkingConfig_ConnectionUserData;
constexpr auto SendRateMin = k_ESteamNetworkingConfig_SendRateMin;
constexpr auto SendRateMax = k_ESteamNetworkingConfig_SendRateMax;
constexpr auto NagleTime = k_ESteamNetworkingConfig_NagleTime;
constexpr auto IP_AllowWithoutAuth = k_ESteamNetworkingConfig_IP_AllowWithoutAuth;
constexpr auto MTU_PacketSize = k_ESteamNetworkingConfig_MTU_PacketSize;
constexpr auto MTU_DataSize = k_ESteamNetworkingConfig_MTU_DataSize;
constexpr auto Unencrypted = k_ESteamNetworkingConfig_Unencrypted;
constexpr auto SymmetricConnect = k_ESteamNetworkingConfig_SymmetricConnect;
constexpr auto LocalVirtualPort = k_ESteamNetworkingConfig_LocalVirtualPort;
constexpr auto EnableDiagnosticsUI = k_ESteamNetworkingConfig_EnableDiagnosticsUI;
constexpr auto FakePacketLoss_Send = k_ESteamNetworkingConfig_FakePacketLoss_Send;
constexpr auto FakePacketLoss_Recv = k_ESteamNetworkingConfig_FakePacketLoss_Recv;
constexpr auto FakePacketLag_Send = k_ESteamNetworkingConfig_FakePacketLag_Send;
constexpr auto FakePacketReorder_Send = k_ESteamNetworkingConfig_FakePacketReorder_Send;
constexpr auto FakePacketReorder_Recv = k_ESteamNetworkingConfig_FakePacketReorder_Recv;
constexpr auto FakePacketReorder_Time = k_ESteamNetworkingConfig_FakePacketReorder_Time;
constexpr auto FakePacketDup_Send = k_ESteamNetworkingConfig_FakePacketDup_Send;
constexpr auto FakePacketDup_Recv = k_ESteamNetworkingConfig_FakePacketDup_Recv;
constexpr auto FakePacketDup_TimeMax = k_ESteamNetworkingConfig_FakePacketDup_TimeMax;
constexpr auto PacketTraceMaxBytes = k_ESteamNetworkingConfig_PacketTraceMaxBytes;
constexpr auto FakeRateLimit_Send_Rate = k_ESteamNetworkingConfig_FakeRateLimit_Send_Rate;
constexpr auto FakeRateLimit_Send_Burst = k_ESteamNetworkingConfig_FakeRateLimit_Send_Burst;
constexpr auto FakeRateLimit_Recv_Rate = k_ESteamNetworkingConfig_FakeRateLimit_Recv_Rate;
constexpr auto FakeRateLimit_Recv_Burst = k_ESteamNetworkingConfig_FakeRateLimit_Recv_Burst;
constexpr auto Callback_ConnectionStatusChanged =
  k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged;
constexpr auto Callback_AuthStatusChanged = k_ESteamNetworkingConfig_Callback_AuthStatusChanged;
constexpr auto Callback_RelayNetworkStatusChanged =
  k_ESteamNetworkingConfig_Callback_RelayNetworkStatusChanged;
constexpr auto Callback_MessagesSessionRequest =
  k_ESteamNetworkingConfig_Callback_MessagesSessionRequest;
constexpr auto Callback_MessagesSessionFailed =
  k_ESteamNetworkingConfig_Callback_MessagesSessionFailed;
constexpr auto Callback_CreateConnectionSignaling =
  k_ESteamNetworkingConfig_Callback_CreateConnectionSignaling;
constexpr auto Callback_FakeIPResult = k_ESteamNetworkingConfig_Callback_FakeIPResult;
constexpr auto P2P_STUN_ServerList = k_ESteamNetworkingConfig_P2P_STUN_ServerList;
constexpr auto P2P_Transport_ICE_Enable = k_ESteamNetworkingConfig_P2P_Transport_ICE_Enable;
constexpr auto P2P_Transport_ICE_Penalty = k_ESteamNetworkingConfig_P2P_Transport_ICE_Penalty;
constexpr auto P2P_Transport_SDR_Penalty = k_ESteamNetworkingConfig_P2P_Transport_SDR_Penalty;
constexpr auto P2P_TURN_ServerList = k_ESteamNetworkingConfig_P2P_TURN_ServerList;
constexpr auto P2P_TURN_UserList = k_ESteamNetworkingConfig_P2P_TURN_UserList;
constexpr auto P2P_TURN_PassList = k_ESteamNetworkingConfig_P2P_TURN_PassList;
constexpr auto P2P_Transport_ICE_Implementation =
  k_ESteamNetworkingConfig_P2P_Transport_ICE_Implementation;
constexpr auto SDRClient_ConsecutitivePingTimeoutsFailInitial =
  k_ESteamNetworkingConfig_SDRClient_ConsecutitivePingTimeoutsFailInitial;
constexpr auto SDRClient_ConsecutitivePingTimeoutsFail =
  k_ESteamNetworkingConfig_SDRClient_ConsecutitivePingTimeoutsFail;
constexpr auto SDRClient_MinPingsBeforePingAccurate =
  k_ESteamNetworkingConfig_SDRClient_MinPingsBeforePingAccurate;
constexpr auto SDRClient_SingleSocket = k_ESteamNetworkingConfig_SDRClient_SingleSocket;
constexpr auto SDRClient_ForceRelayCluster = k_ESteamNetworkingConfig_SDRClient_ForceRelayCluster;
constexpr auto SDRClient_DebugTicketAddress = k_ESteamNetworkingConfig_SDRClient_DebugTicketAddress;
constexpr auto SDRClient_ForceProxyAddr = k_ESteamNetworkingConfig_SDRClient_ForceProxyAddr;
constexpr auto SDRClient_FakeClusterPing = k_ESteamNetworkingConfig_SDRClient_FakeClusterPing;
constexpr auto LogLevel_AckRTT = k_ESteamNetworkingConfig_LogLevel_AckRTT;
constexpr auto LogLevel_PacketDecode = k_ESteamNetworkingConfig_LogLevel_PacketDecode;
constexpr auto LogLevel_Message = k_ESteamNetworkingConfig_LogLevel_Message;
constexpr auto LogLevel_PacketGaps = k_ESteamNetworkingConfig_LogLevel_PacketGaps;
constexpr auto LogLevel_P2PRendezvous = k_ESteamNetworkingConfig_LogLevel_P2PRendezvous;
constexpr auto LogLevel_SDRRelayPings = k_ESteamNetworkingConfig_LogLevel_SDRRelayPings;
constexpr auto DELETED_EnumerateDevVars = k_ESteamNetworkingConfig_DELETED_EnumerateDevVars;
constexpr auto alue__Force32Bit = k_ESteamNetworkingConfigValue__Force32Bit;
} // namespace ESteamNetworkingConfig

namespace EResult
{
constexpr auto None = k_EResultNone;
constexpr auto OK = k_EResultOK;                                 // success
constexpr auto Fail = k_EResultFail;                             // generic failure
constexpr auto NoConnection = k_EResultNoConnection;             // no/failed network connection
                                                                 //	k_EResultNoConnectionRetry = 4,
                                                                 //// OBSOLETE - removed
constexpr auto InvalidPassword = k_EResultInvalidPassword;       // password/ticket is invalid
constexpr auto LoggedInElsewhere = k_EResultLoggedInElsewhere;   // same user logged in elsewhere
constexpr auto InvalidProtocolVer = k_EResultInvalidProtocolVer; // protocol version is incorrect
constexpr auto InvalidParam = k_EResultInvalidParam;             // a parameter is incorrect
constexpr auto FileNotFound = k_EResultFileNotFound;             // file was not found
constexpr auto Busy = k_EResultBusy;                       // called method busy - action not taken
constexpr auto InvalidState = k_EResultInvalidState;       // called object was in an invalid state
constexpr auto InvalidName = k_EResultInvalidName;         // name is invalid
constexpr auto InvalidEmail = k_EResultInvalidEmail;       // email is invalid
constexpr auto DuplicateName = k_EResultDuplicateName;     // name is not unique
constexpr auto AccessDenied = k_EResultAccessDenied;       // access is denied
constexpr auto Timeout = k_EResultTimeout;                 // operation timed out
constexpr auto Banned = k_EResultBanned;                   // VAC2 banned
constexpr auto AccountNotFound = k_EResultAccountNotFound; // account not found
constexpr auto InvalidSteamID = k_EResultInvalidSteamID;   // steamID is invalid
constexpr auto ServiceUnavailable =
  k_EResultServiceUnavailable;                     // The requested service is currently unavailable
constexpr auto NotLoggedOn = k_EResultNotLoggedOn; // The user is not logged on
constexpr auto Pending =
  k_EResultPending; // Request is pending (may be in process, or waiting on third party)
constexpr auto EncryptionFailure = k_EResultEncryptionFailure; // Encryption or Decryption failed
constexpr auto InsufficientPrivilege = k_EResultInsufficientPrivilege; // Insufficient privilege
constexpr auto LimitExceeded = k_EResultLimitExceeded;                 // Too much of a good thing
constexpr auto Revoked =
  k_EResultRevoked; // Access has been revoked (used for revoked guest passes)
constexpr auto Expired =
  k_EResultExpired; // License/Guest pass the user is trying to access is expired
constexpr auto AlreadyRedeemed = k_EResultAlreadyRedeemed; // Guest pass has already been redeemed
                                                           // by account, cannot be acked again
constexpr auto DuplicateRequest =
  k_EResultDuplicateRequest; // The request is a duplicate and the action has
                             // already occurred in the past, ignored this time
constexpr auto AlreadyOwned = k_EResultAlreadyOwned; // All the games in this guest pass redemption
                                                     // request are already owned by the user
constexpr auto IPNotFound = k_EResultIPNotFound;     // IP address not found
constexpr auto PersistFailed = k_EResultPersistFailed; // failed to write change to the data store
constexpr auto LockingFailed =
  k_EResultLockingFailed; // failed to acquire access lock for this operation
constexpr auto LogonSessionReplaced = k_EResultLogonSessionReplaced;
constexpr auto ConnectFailed = k_EResultConnectFailed;
constexpr auto HandshakeFailed = k_EResultHandshakeFailed;
constexpr auto IOFailure = k_EResultIOFailure;
constexpr auto RemoteDisconnect = k_EResultRemoteDisconnect;
constexpr auto ShoppingCartNotFound =
  k_EResultShoppingCartNotFound;           // failed to find the shopping cart requested
constexpr auto Blocked = k_EResultBlocked; // a user didn't allow it
constexpr auto Ignored = k_EResultIgnored; // target is ignoring sender
constexpr auto NoMatch = k_EResultNoMatch; // nothing matching the request found
constexpr auto AccountDisabled = k_EResultAccountDisabled;
constexpr auto ServiceReadOnly =
  k_EResultServiceReadOnly; // this service is not accepting content changes right now
constexpr auto AccountNotFeatured =
  k_EResultAccountNotFeatured; // account doesn't have value, so this feature isn't available
constexpr auto AdministratorOK =
  k_EResultAdministratorOK; // allowed to take this action, but only because requester is admin
constexpr auto ContentVersion = k_EResultContentVersion; // A Version mismatch in content
                                                         // transmitted within the Steam protocol.
constexpr auto TryAnotherCM = k_EResultTryAnotherCM; // The current CM can't service the user making
                                                     // a request, user should try another.
constexpr auto PasswordRequiredToKickSession =
  k_EResultPasswordRequiredToKickSession; // You are already logged in elsewhere, this cached
                                          // credential login has failed.
constexpr auto AlreadyLoggedInElsewhere =
  k_EResultAlreadyLoggedInElsewhere; // You are already logged in elsewhere, you must wait
constexpr auto Suspended =
  k_EResultSuspended; // Long running operation (content download) suspended/paused
constexpr auto Cancelled =
  k_EResultCancelled; // Operation canceled (typically by user: content download)
constexpr auto DataCorruption =
  k_EResultDataCorruption; // Operation canceled because data is ill formed or unrecoverable
constexpr auto DiskFull = k_EResultDiskFull; // Operation canceled - not enough disk space.
constexpr auto RemoteCallFailed = k_EResultRemoteCallFailed; // an remote call or IPC call failed
constexpr auto PasswordUnset =
  k_EResultPasswordUnset; // Password could not be verified as it's unset server side
constexpr auto ExternalAccountUnlinked =
  k_EResultExternalAccountUnlinked;                          // External account (PSN, Facebook...)
                                                             // is not linked to a Steam account
constexpr auto PSNTicketInvalid = k_EResultPSNTicketInvalid; // PSN ticket was invalid
constexpr auto ExternalAccountAlreadyLinked =
  k_EResultExternalAccountAlreadyLinked; // External account (PSN, Facebook...) is already
                                         // linked to some other account, must explicitly
                                         // request to replace/delete the link first
constexpr auto RemoteFileConflict =
  k_EResultRemoteFileConflict; // The sync cannot resume due to a conflict
                               // between the local and remote files
constexpr auto IllegalPassword =
  k_EResultIllegalPassword; // The requested new password is not legal
constexpr auto SameAsPreviousValue =
  k_EResultSameAsPreviousValue; // new value is the same as the old one (
                                // secret question and answer )
constexpr auto AccountLogonDenied =
  k_EResultAccountLogonDenied; // account login denied due to 2nd factor authentication failure
constexpr auto CannotUseOldPassword =
  k_EResultCannotUseOldPassword; // The requested new password is not legal
constexpr auto InvalidLoginAuthCode =
  k_EResultInvalidLoginAuthCode; // account login denied due to auth code invalid
constexpr auto AccountLogonDeniedNoMail =
  k_EResultAccountLogonDeniedNoMail; // account login denied due to 2nd factor auth failure -
                                     // and no mail has been sent
constexpr auto HardwareNotCapableOfIPT = k_EResultHardwareNotCapableOfIPT; //
constexpr auto IPTInitError = k_EResultIPTInitError;                       //
constexpr auto ParentalControlRestricted =
  k_EResultParentalControlRestricted; // operation failed due to parental control restrictions
                                      // for current user
constexpr auto FacebookQueryError = k_EResultFacebookQueryError; // Facebook query returned an error
constexpr auto ExpiredLoginAuthCode =
  k_EResultExpiredLoginAuthCode; // account login denied due to auth code expired
constexpr auto IPLoginRestrictionFailed = k_EResultIPLoginRestrictionFailed;
constexpr auto AccountLockedDown = k_EResultAccountLockedDown;
constexpr auto AccountLogonDeniedVerifiedEmailRequired =
  k_EResultAccountLogonDeniedVerifiedEmailRequired;
constexpr auto NoMatchingURL = k_EResultNoMatchingURL;
constexpr auto BadResponse = k_EResultBadResponse; // parse failure, missing field, etc.
constexpr auto RequirePasswordReEntry =
  k_EResultRequirePasswordReEntry; // The user cannot complete the action
                                   // until they re-enter their password
constexpr auto ValueOutOfRange =
  k_EResultValueOutOfRange; // the value entered is outside the acceptable range
constexpr auto UnexpectedError =
  k_EResultUnexpectedError; // something happened that we didn't expect to ever happen
constexpr auto Disabled =
  k_EResultDisabled; // The requested service has been configured to be unavailable
constexpr auto InvalidCEGSubmission =
  k_EResultInvalidCEGSubmission; // The set of files submitted to the CEG server are not valid !
constexpr auto RestrictedDevice =
  k_EResultRestrictedDevice; // The device being used is not allowed to perform this action
constexpr auto RegionLocked =
  k_EResultRegionLocked; // The action could not be complete because it is region restricted
constexpr auto RateLimitExceeded =
  k_EResultRateLimitExceeded; // Temporary rate limit exceeded, try again later, different
                              // from k_EResultLimitExceeded which may be permanent
constexpr auto AccountLoginDeniedNeedTwoFactor =
  k_EResultAccountLoginDeniedNeedTwoFactor; // Need two-factor code to login
constexpr auto ItemDeleted =
  k_EResultItemDeleted; // The thing we're trying to access has been deleted
constexpr auto AccountLoginDeniedThrottle =
  k_EResultAccountLoginDeniedThrottle; // login attempt failed, try to throttle response to
                                       // possible attacker
constexpr auto TwoFactorCodeMismatch = k_EResultTwoFactorCodeMismatch; // two factor code mismatch
constexpr auto TwoFactorActivationCodeMismatch =
  k_EResultTwoFactorActivationCodeMismatch; // activation code for two-factor didn't match
constexpr auto AccountAssociatedToMultiplePartners =
  k_EResultAccountAssociatedToMultiplePartners;    // account has been associated with multiple
                                                   // partners
constexpr auto NotModified = k_EResultNotModified; // data not modified
constexpr auto NoMobileDevice =
  k_EResultNoMobileDevice; // the account does not have a mobile device associated with it
constexpr auto TimeNotSynced =
  k_EResultTimeNotSynced; // the time presented is out of range or tolerance
constexpr auto SmsCodeFailed =
  k_EResultSmsCodeFailed; // SMS code failure (no match, none pending, etc.)
constexpr auto AccountLimitExceeded =
  k_EResultAccountLimitExceeded; // Too many accounts access this resource
constexpr auto AccountActivityLimitExceeded =
  k_EResultAccountActivityLimitExceeded; // Too many changes to this account
constexpr auto PhoneActivityLimitExceeded =
  k_EResultPhoneActivityLimitExceeded; // Too many changes to this phone
constexpr auto RefundToWallet =
  k_EResultRefundToWallet; // Cannot refund to payment method, must use wallet
constexpr auto EmailSendFailure = k_EResultEmailSendFailure; // Cannot send an email
constexpr auto NotSettled = k_EResultNotSettled; // Can't perform operation till payment has settled
constexpr auto NeedCaptcha = k_EResultNeedCaptcha; // Needs to provide a valid captcha
constexpr auto GSLTDenied =
  k_EResultGSLTDenied; // a game server login token owned by this token's owner has been banned
constexpr auto GSOwnerDenied =
  k_EResultGSOwnerDenied; // game server owner is denied for other reason (account
                          // lock, community ban, vac ban, missing phone)
constexpr auto InvalidItemType =
  k_EResultInvalidItemType; // the type of thing we were requested to act on is invalid
constexpr auto IPBanned =
  k_EResultIPBanned; // the ip address has been banned from taking this action
constexpr auto GSLTExpired =
  k_EResultGSLTExpired; // this token has expired from disuse; can be reset for use
constexpr auto InsufficientFunds =
  k_EResultInsufficientFunds; // user doesn't have enough wallet funds to complete the action
constexpr auto TooManyPending =
  k_EResultTooManyPending; // There are too many of this thing pending already
constexpr auto NoSiteLicensesFound = k_EResultNoSiteLicensesFound; // No site licenses found
constexpr auto WGNetworkSendExceeded =
  k_EResultWGNetworkSendExceeded; // the WG couldn't send a response because we
                                  // exceeded max network send size
constexpr auto AccountNotFriends = k_EResultAccountNotFriends;   // the user is not mutually friends
constexpr auto LimitedUserAccount = k_EResultLimitedUserAccount; // the user is limited
constexpr auto CantRemoveItem = k_EResultCantRemoveItem;         // item can't be removed
constexpr auto AccountDeleted = k_EResultAccountDeleted;         // account has been deleted
constexpr auto ExistingUserCancelledLicense =
  k_EResultExistingUserCancelledLicense; // A license for this already exists, but cancelled
constexpr auto CommunityCooldown =
  k_EResultCommunityCooldown; // access is denied because of a community cooldown (probably
                              // from support profile data resets)
constexpr auto NoLauncherSpecified =
  k_EResultNoLauncherSpecified; // No launcher was specified, but a launcher was needed to
                                // choose correct realm for operation.
constexpr auto MustAgreeToSSA =
  k_EResultMustAgreeToSSA; // User must agree to china SSA or global SSA before login
constexpr auto LauncherMigrated =
  k_EResultLauncherMigrated; // The specified launcher type is no longer supported; the user
                             // should be directed elsewhere
constexpr auto SteamRealmMismatch =
  k_EResultSteamRealmMismatch; // The user's realm does not match the realm
                               // of the requested resource
constexpr auto InvalidSignature = k_EResultInvalidSignature; // signature check did not match
constexpr auto ParseFailure = k_EResultParseFailure;         // Failed to parse input
constexpr auto NoVerifiedPhone =
  k_EResultNoVerifiedPhone; // account does not have a verified phone number
constexpr auto InsufficientBattery =
  k_EResultInsufficientBattery; // user device doesn't have enough battery
                                // charge currently to complete the action
constexpr auto ChargerRequired = k_EResultChargerRequired; // The operation requires a charger to be
                                                           // plugged in, which wasn't present
constexpr auto CachedCredentialInvalid =
  k_EResultCachedCredentialInvalid; // Cached credential was invalid - user must reauthenticate
}; // namespace EResult

} // namespace GNS
