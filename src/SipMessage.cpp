#include <sstream>
#include <iostream>
#include <regex>

#include <easylogging++.h>
#include <asio/ip/udp.hpp>


#include "SipMessage.hpp"
#include "ErrorCode.hpp"
#include "ExitException.hpp"

SipMessage::SipMessage(const char * rawStringMessage) {
    std::istringstream iss(rawStringMessage);
    std::string tmp;
    std::getline(iss, tmp);
    this->type = parseType(tmp);
    this->startString = tmp;
    if (SipMessageType::Unknown == this->type) {
        std::string description = "Unknown message type";
        throw ExitException(ErrorCode::PARSING_ERROR);
    }
    bool isEmptyStringContained = false;
    while(std::getline(iss, tmp)) {
        //Todo: СДЕЛАТЬ НАРМАЛЬНА
        if("" == tmp) {
            isEmptyStringContained = true;
            while(std::getline(iss, tmp)) {
                body += tmp;
                body += '\n';
                /* SDP parsing is not necessary now
                std::istringstream line(tmp);
                std::string key;
                std::string value;
                std::getline(line, key, '=');
                std::getline(line, value);
                this->body.insert(std::make_pair(key,value));*/
            }
            break;
        }
        std::istringstream line(tmp);
        std::string key;
        std::string value;
        std::getline(line, key, ':');
        std::getline(line, value);

        value.erase(0, value.find_first_not_of(' '));       //prefixing spaces

        this->headers.insert(std::make_pair(key,value));
    }
    if (!isEmptyStringContained) {
        std::string description = "Empty string is missed in SIP message";
        throw ExitException(ErrorCode::PARSING_ERROR, description);
    }

    if (SipMessageType::Request == type) {
        MethodType method = parseMethod(startString);
        if(MethodType::NONE == method) {
            std::string description = "Method in request is not parsed. Starting string " + startString;
            throw ExitException(ErrorCode::PARSING_ERROR, description);
        }
        this->method = method;
    }

    if(headers.find("Contact") != headers.end()) {
        auto parsedContact = parseContact(headers.find("Contact")->second);
        senderId = parsedContact.first;
        senderEndPoint = parsedContact.second;
    }
}

SipMessage::SipMessage(std::string startString, std::multimap<std::string, std::string> headers,
                       std::string body):
    startString(startString), headers(headers), body(body)
{
    //
}


SipMessage::operator std::string() const {
    std::string result;
    result += startString;
    result += "\n";
    for(auto elem: headers) {
        result += elem.first;
        result += ": ";
        result += elem.second;
        result += "\n";
    }
    result += "\n";
    result += body;
    /*SDP handling is not necessary now
        for(auto elem: body) {
        result += elem.first;
        result += "=";
        result += elem.second;
        result += "\n";
    }*/
    return result;
}

SipMessageType SipMessage::getSipMessageType() {
    return type;
}

MethodType SipMessage::getMethod() {
    return method;
}

SipMessageType SipMessage::parseType(std::string& str) {
    static std::regex request("[A-Z]{3,9} .+ SIP\\/2\\.0");
    static std::regex response("SIP\\/2\\.0 \\d{3} \\w+");
    if (std::regex_match(str, request)) {
        return SipMessageType::Request;
    }
    else if (std::regex_match(str, response)) {
        return SipMessageType::Response;
    }
    else {
        return SipMessageType::Unknown;
    }
}

MethodType SipMessage::parseMethod(std::string& str) {
    static std::regex methodRegex("([A-Z]{3,9}) .+");
    std::smatch matched;
    if (std::regex_match(str, matched, methodRegex)) {
        auto matchedString = matched[1];
        if ("REGISTER" == matchedString) {
            return MethodType::REGISTER;
        } //TODO: Other method
        else {
            return MethodType::NONE;
        }
    }

    else {
        std::string description = "Method is not found";
        throw ExitException(ErrorCode::PARSING_ERROR, description);
    }
}

std::string SipMessage::getMethod(MethodType methodType) {
    static std::unordered_map<MethodType, std::string> methods;
    methods[MethodType::NONE] = "NONE";
    methods[MethodType::REGISTER] = "REGISTER";
    return methods[methodType];
}

std::pair<std::string, asio::ip::udp::endpoint> SipMessage::parseContact(std::string& str) {
    static std::regex parseContactRegext("<?(\\w+:)?(\\w+)@((\\d{1,3}\\.){3}\\d{1,3}):?(\\d{4,6})?>?.*");
    std::string senderId;
    asio::ip::udp::endpoint senderEndPoint;
    std::smatch matched;
    if (std::regex_match(str, matched, parseContactRegext)) {
        LOG(DEBUG) << "\"Contact\" header is valid";
        senderId = matched[2];
        auto senderIpAddress = matched[3];
        auto senderPort = matched[5];
        senderEndPoint = asio::ip::udp::endpoint(asio::ip::address::from_string(senderIpAddress), std::stoi(senderPort));
        LOG(DEBUG) << "Sender endPoint: " << senderIpAddress << ":" << senderPort;
    }
    else {
        std::string description = "\"Contact\" header is not valid";
        throw ExitException(ErrorCode::PARSING_ERROR, description);
    }
    return std::make_pair(senderId, senderEndPoint);
}

std::string SipMessage::getSenderId() {
    return senderId;
}

asio::ip::udp::endpoint SipMessage::getSenderEndPoint() {
    return senderEndPoint;
}

std::multimap<std::string, std::string> SipMessage::getHeaders() {
    return headers;
}

std::string SipMessage::getBody() {
    return body;
}
