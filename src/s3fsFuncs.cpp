#include "main.hpp"
#include "config.h"
#include <string.h>

static Aws::SDKOptions& GetSDKOptions()
{
	static Aws::SDKOptions	options;
	return options;
}

const char* VersionS3fsCredential(bool detail)
{
        const char short_version_form[]  = "s3fs-rgw-sts-lib : Version %s (%s)";
        const char detail_version_form[] =
                "s3fs-rgw-sts-lib : Version %s (%s)\n"
                "s3fs-fuse credential I/F library for Ceph RADOS Gateway using OIDC AuthN/AuthZ\n"
                "Copyright 2022 Federico Fornari <federico.fornari@cnaf.infn.it>\n";

        static char short_version_string[128];
        static char detail_version_string[256];
        static bool is_init = false;

        if(!is_init){
                is_init = true;
                sprintf(short_version_string, short_version_form, product_version, commit_hash_version);
                sprintf(detail_version_string, detail_version_form, product_version, commit_hash_version);
        }
        if(detail){
                return detail_version_string;
        }else{
                return short_version_string;
        }
}

bool InitS3fsCredential(const char* popts, char** pperrstr)
{
        if(pperrstr){
                *pperrstr = NULL;
        }

        //
        // Check option arguments and set it
        //
        Aws::SDKOptions& options = GetSDKOptions();

	if(popts && 0 < strlen(popts)){
		if(0 == strcasecmp(popts, "Off")){
			options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Off;
		}else if(0 == strcasecmp(popts, "Fatal")){
			options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Fatal;
		}else if(0 == strcasecmp(popts, "Error")){
			options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
		}else if(0 == strcasecmp(popts, "Warn")){
			options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
		}else if(0 == strcasecmp(popts, "Info")){
			options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
		}else if(0 == strcasecmp(popts, "Debug")){
			options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;
		}else if(0 == strcasecmp(popts, "Trace")){
			options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
		}else{
			if(pperrstr){
				*pperrstr = strdup("Unknown option(Aws::Utils::Logging::LogLevel) is specified.");
			}
			return false;
		}
	}

        //
        // Initalize
        //
        Aws::InitAPI(options);

        return true;
}

bool FreeS3fsCredential(char** pperrstr)
{
        if(pperrstr){
                *pperrstr = NULL;
        }

        //
        // Shutdown
        //
        Aws::ShutdownAPI(GetSDKOptions());

        return true;
}

bool UpdateS3fsCredential(char** ppaccess_key_id, char** ppserect_access_key, char** ppaccess_token, long long* ptoken_expire, char** pperrstr)
{
        if(!ppaccess_key_id || !ppserect_access_key || !ppaccess_token || !ptoken_expire){
                if(pperrstr){
                        *pperrstr = strdup("Some parameters are wrong(NULL).");
                }
                return false;
        }
        if(pperrstr){
                *pperrstr = NULL;
        }

        auto result = true;
        Aws::SDKOptions& options = GetSDKOptions();

        auto roleArn = std::getenv("ROLE_ARN") ? std::getenv("ROLE_ARN") : "";
        auto roleSessionName = std::getenv("ROLE_NAME") ? std::getenv("ROLE_NAME") : "";
        auto KCHost = std::getenv("KC_HOST") ? std::getenv("KC_HOST") : "localhost";
        auto KCRealm = std::getenv("KC_REALM") ? std::getenv("KC_REALM") : "";
        auto clientId = std::getenv("CLIENT_ID") ? std::getenv("CLIENT_ID") : "";
        auto clientSecret = std::getenv("CLIENT_SECRET") ? std::getenv("CLIENT_SECRET") : "";
        Aws::String endpointOverride = std::getenv("ENDPOINT_URL") ? std::getenv("ENDPOINT_URL") : "localhost";
        Aws::String region = std::getenv("REGION_NAME") ? std::getenv("REGION_NAME") : "";

        Aws::String webIdentityToken = getOIDCToken(KCHost, KCRealm, clientId, clientSecret);

        Aws::Auth::AWSCredentials credentials;
        Aws::Client::ClientConfiguration clientConfig;

        configureClient(clientConfig, endpointOverride, region);

        if (!AwsDoc::STS::assumeRoleWithWebIdentity(roleArn, roleSessionName, webIdentityToken, credentials, clientConfig))
        {
            return 1;
        }

        // Get credentials
	Aws::String accessKeyId	        = credentials.GetAWSAccessKeyId();
	Aws::String secretKey           = credentials.GetAWSSecretKey();
	Aws::String sessionToken        = credentials.GetSessionToken();
	Aws::Utils::DateTime expiration = Aws::Utils::DateTime::Now() + std::chrono::milliseconds(3600000);

	// Set result buffers
	*ppaccess_key_id     = strdup(accessKeyId.c_str());
	*ppserect_access_key = strdup(secretKey.c_str());
	*ppaccess_token	     = strdup(sessionToken.c_str());
	*ptoken_expire       = expiration.Millis() / 1000;

	// For debug
	if(Aws::Utils::Logging::LogLevel::Info <= options.loggingOptions.logLevel){
		std::cout << "[s3fsrgwsts] : Access Key Id = " << accessKeyId	<< std::endl;
		std::cout << "[s3fsrgwsts] : Secret Key    = " << secretKey	<< std::endl;
		std::cout << "[s3fsrgwsts] : Session Token = " << sessionToken	<< std::endl;
		std::cout << "[s3fsrgwsts] : expiration    = " << expiration.ToLocalTimeString(Aws::Utils::DateFormat::ISO_8601) << std::endl;
	}

        if(!*ppaccess_key_id || !*ppserect_access_key || !*ppaccess_token){
                if(pperrstr){
                        *pperrstr = strdup("Could not allocate memory.");
                }
                if(!*ppaccess_key_id){
                        free(*ppaccess_key_id);
                        *ppaccess_key_id = NULL;
                }
                if(!*ppserect_access_key){
                        free(*ppserect_access_key);
                        *ppserect_access_key = NULL;
                }
                if(!*ppaccess_token){
                        free(*ppaccess_token);
                        *ppaccess_token = NULL;
                }
                *ptoken_expire = 0;

                result = false;
        }

        return result;
}
