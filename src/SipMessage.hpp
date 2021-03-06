#pragma once
#ifndef SIPSERVER_SIPMESSAGE_HPP
#define SIPSERVER_SIPMESSAGE_HPP

#include <map>

#include <MethodType.hpp>

#include <asio/ip/udp.hpp>

enum class SipMessageType {
    Request = 0,
    Response = 1,
    Unknown = 2
};

class SipMessage {
    friend class SipServer;

    private:
        std::string startString;
        std::multimap<std::string, std::string> headers;
        std::string body;
        SipMessageType type;
        MethodType method = MethodType::NONE;
        std::string senderId;
        asio::ip::udp::endpoint senderEndPoint;

        static SipMessageType parseType(std::string&);
        static MethodType parseMethod(std::string&);
        std::pair<std::string, asio::ip::udp::endpoint> parseContact(std::string& str);

    public:
        SipMessage() = default;
        SipMessage(const char* rowStringMessage);
        SipMessage(std::string startString,
                   std::multimap<std::string, std::string> headers,
                   std::string body);
        SipMessageType getSipMessageType();
        MethodType getMethod();
        static std::string getMethod(MethodType);
        std::string getSenderId();
        asio::ip::udp::endpoint getSenderEndPoint();

        std::multimap<std::string, std::string> getHeaders();
        std::string getBody();

        operator std::string() const;
};


#endif //SIPSERVER_SIPMESSAGE_HPP
