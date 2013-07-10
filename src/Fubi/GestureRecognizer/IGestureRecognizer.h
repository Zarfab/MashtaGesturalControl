// ****************************************************************************************
//
// Fubi Gesture Recognizer
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#pragma once

#include "../FubiUtils.h"
#include "../FubiUser.h"

#include <map>

class IGestureRecognizer
{
public:
	IGestureRecognizer() : m_ignoreOnTrackingError(false), m_minConfidence(0.51f) {}
	IGestureRecognizer(bool ignoreOnTrackingError, float minconfidence) : m_ignoreOnTrackingError(ignoreOnTrackingError)
	{ m_minConfidence = (minconfidence >= 0) ? minconfidence : 0.51f; }
	virtual ~IGestureRecognizer() {}

	virtual Fubi::RecognitionResult::Result recognizeOn(FubiUser* user) = 0;
	virtual IGestureRecognizer* clone() = 0;

	bool m_ignoreOnTrackingError;
	float m_minConfidence;
};