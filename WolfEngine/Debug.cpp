#include "Debug.h"

void Wolf::Debug::sendError(std::string errorMessage)
{
	m_callback(Severity::ERROR, std::move(errorMessage));
}

void Wolf::Debug::setCallback(std::function<void(Severity, std::string)> callback)
{
	m_callback = std::move(callback);
}
