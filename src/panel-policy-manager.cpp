/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "panel-policy-manager.h"

#include <ndn.cxx/security/certificate/identity-certificate.h>
#include <ndn.cxx/security/cache/ttl-certificate-cache.h>
#include <boost/bind.hpp>

#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("PanelPolicyManager");

PanelPolicyManager::PanelPolicyManager(const int & stepLimit,
                                       Ptr<CertificateCache> certificateCache)
  : m_stepLimit(stepLimit)
  , m_certificateCache(certificateCache)
  , m_localPrefixRegex(Ptr<Regex>(new Regex("^<local><ndn><prefix><><>$")))
{
  if(NULL == m_certificateCache)
    m_certificateCache = Ptr<security::CertificateCache>(new security::TTLCertificateCache());

  m_invitationDataSigningRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", 
                                                                               "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT><>$", 
                                                                               "==", "\\1", "\\1\\2", true));
  
  m_dskRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT><>$", 
							     "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
							     "==", "\\1", "\\1\\2", true));
  
  m_endorseeRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<DNS>]*)<DNS><>*<ENDORSEE><>$", 
                                                                  "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                                                  "==", "\\1", "\\1\\2", true));
  
  m_kskRegex = Ptr<Regex>(new Regex("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT><>$", "\\1\\2"));

  m_keyNameRegex = Ptr<Regex>(new Regex("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", "\\1\\2"));

  m_signingCertificateRegex = Ptr<Regex>(new Regex("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", "\\1"));
}

bool 
PanelPolicyManager::skipVerifyAndTrust (const Data & data)
{
  if(m_localPrefixRegex->match(data.getName()))
    return true;
  
  return false;
}

bool
PanelPolicyManager::requireVerify (const Data & data)
{
  // if(m_invitationDataRule->matchDataName(data))
  //   return true;
  if(m_kskRegex->match(data.getName()))
     return true;
  if(m_dskRule->matchDataName(data))
    return true;

  if(m_endorseeRule->matchDataName(data))
    return true;


  return false;
}

Ptr<ValidationRequest>
PanelPolicyManager::checkVerificationPolicy(Ptr<Data> data, 
						 const int & stepCount, 
						 const DataCallback& verifiedCallback,
						 const UnverifiedCallback& unverifiedCallback)
{
  if(m_stepLimit == stepCount)
    {
      _LOG_ERROR("Reach the maximum steps of verification!");
      unverifiedCallback(data);
      return NULL;
    }

  Ptr<const signature::Sha256WithRsa> sha256sig = boost::dynamic_pointer_cast<const signature::Sha256WithRsa> (data->getSignature());    

  if(KeyLocator::KEYNAME != sha256sig->getKeyLocator().getType())
    {
      _LOG_ERROR("Keylocator is not name!");
      unverifiedCallback(data);
      return NULL;
    }

  const Name & keyLocatorName = sha256sig->getKeyLocator().getKeyName();

  if(m_kskRegex->match(data->getName()))
    {
      Name keyName = m_kskRegex->expand();
      map<Name, Publickey>::iterator it = m_trustAnchors.find(keyName);
      if(m_trustAnchors.end() != it)
        {
          // _LOG_DEBUG("found key!");
          Ptr<IdentityCertificate> identityCertificate = Ptr<IdentityCertificate>(new IdentityCertificate(*data));
          if(it->second.getKeyBlob() == identityCertificate->getPublicKeyInfo().getKeyBlob())
            {
              verifiedCallback(data);
            }
          else
            unverifiedCallback(data);
        }
      else
        unverifiedCallback(data);

      return NULL;
    }

  if(m_dskRule->satisfy(*data))
    {
      m_keyNameRegex->match(keyLocatorName);
      Name keyName = m_keyNameRegex->expand();

      if(m_trustAnchors.end() != m_trustAnchors.find(keyName))
        if(verifySignature(*data, m_trustAnchors[keyName]))
          verifiedCallback(data);
        else
          unverifiedCallback(data);
      else
        unverifiedCallback(data);

      return NULL;	
    }

  if(m_endorseeRule->satisfy(*data))
    {
      m_keyNameRegex->match(keyLocatorName);
      Name keyName = m_keyNameRegex->expand();
      if(m_trustAnchors.end() != m_trustAnchors.find(keyName))
        if(verifySignature(*data, m_trustAnchors[keyName]))
          verifiedCallback(data);
        else
          unverifiedCallback(data);
      else
        unverifiedCallback(data);

      return NULL;
    }

  _LOG_DEBUG("Unverified!");

  unverifiedCallback(data);
  return NULL;
}

bool 
PanelPolicyManager::checkSigningPolicy(const Name & dataName, const Name & certificateName)
{
  return m_invitationDataSigningRule->satisfy(dataName, certificateName);
}

Name 
PanelPolicyManager::inferSigningIdentity(const Name & dataName)
{
  if(m_signingCertificateRegex->match(dataName))
    return m_signingCertificateRegex->expand();
  else
    return Name();
}

void
PanelPolicyManager::addTrustAnchor(const EndorseCertificate& selfEndorseCertificate)
{ 
  // _LOG_DEBUG("Add Anchor: " << selfEndorseCertificate.getPublicKeyName().toUri());
  m_trustAnchors.insert(pair <Name, Publickey > (selfEndorseCertificate.getPublicKeyName(), selfEndorseCertificate.getPublicKeyInfo())); 
}

void
PanelPolicyManager::removeTrustAnchor(const Name& keyName)
{  
  m_trustAnchors.erase(keyName); 
}

Ptr<Publickey>
PanelPolicyManager::getTrustedKey(const ndn::Name& inviterCertName)
{
  Name keyLocatorName = inviterCertName.getPrefix(inviterCertName.size()-1);
  m_keyNameRegex->match(keyLocatorName);
  Name keyName = m_keyNameRegex->expand();

  if(m_trustAnchors.end() != m_trustAnchors.find(keyName))
    return Ptr<Publickey>(new Publickey(m_trustAnchors[keyName]));
  return NULL;
}
