// ****************************************************************************************
//
// Fubi Combination Recognizer
// ---------------------------------------------------------
// Copyright (C) 2010-2013 Felix Kistler 
// 
// This software is distributed under the terms of the Eclipse Public License v1.0.
// A copy of the license may be obtained at: http://www.eclipse.org/org/documents/epl-v10.html
// 
// ****************************************************************************************

#pragma once

#include "../FubiPredefinedGestures.h"
#include "IGestureRecognizer.h"

#include <vector>

// A state of the combination recognizer
struct RecognitionState
{
	RecognitionState(const std::vector<IGestureRecognizer*>& gestureRecognizers, const std::vector<IGestureRecognizer*>& notRecognizers,
		double minDuration, double maxDuration, double timeForTransition, double maxInterruption, bool noInterrruptionBeforeMinDuration,
		const std::vector<IGestureRecognizer*>& alternativeGestureRecognizers, const std::vector<IGestureRecognizer*>& alternativeNotRecognizers)
		: m_gestures(gestureRecognizers), m_notGestures(notRecognizers),
			m_minDuration(minDuration), m_maxDuration(maxDuration),
			m_timeForTransition(timeForTransition), m_maxInterruptionTime(maxInterruption), m_noInterrruptionBeforeMinDuration(noInterrruptionBeforeMinDuration),
			m_alternativeGestures(alternativeGestureRecognizers), m_alternativeNotGestures(alternativeNotRecognizers)
	{
		if (m_maxInterruptionTime < 0)
			m_maxInterruptionTime = Fubi::clamp<double>(m_timeForTransition, 0, 0.1);
	}

	static const std::vector<IGestureRecognizer*> s_emptyRecVec;
	typedef std::vector<IGestureRecognizer*>::const_iterator GestureIter;
	std::vector<IGestureRecognizer*> m_gestures;
	std::vector<IGestureRecognizer*> m_notGestures;
	std::vector<IGestureRecognizer*> m_alternativeGestures;
	std::vector<IGestureRecognizer*> m_alternativeNotGestures;
	double m_minDuration;
	double m_maxDuration;
	double m_timeForTransition;
	double m_maxInterruptionTime;
	bool m_noInterrruptionBeforeMinDuration;
};

class CombinationRecognizer
{
public:
	CombinationRecognizer(FubiUser* user, Fubi::Combinations::Combination gestureID);
	CombinationRecognizer(const std::string& recognizerName);
	CombinationRecognizer(const CombinationRecognizer& other);

	~CombinationRecognizer();

	// updates the current combination state
	void update();

	// Starts and stops the recognition
	void start();
	void stop();
	void restart() { stop(); start(); }

	// True if a combination start is detected (First state recognized, but last one not yet)
	bool isInProgress() {return m_currentState > -1; }
	// True if the recognizer is currently trying to detect the combination (Last state not yet recognized)
	bool isRunning()	{ return m_running; }
	// True if recognizer is currently trying to detect the combination or has already suceeded (running or recognized, but not stopped)
	bool isActive()		{ return m_running || m_recognized; }
	// True if already recognized, but still waiting for the last state to finish
	bool isWaitingForLastStateFinish()		{ return m_waitUntilLastStateRecognizersStop && m_running && m_minDurationPassed && m_RecognitionStates.size() > 0 && (m_currentState == m_RecognitionStates.size()-1); }
	// @param userStates: vector were the user skeletonData and timestamps are stored for each transition during a recognition that has been successful
	// @param restart: if true, the recognition will automatically restart if it is successful
	// returns true if a combination is completed 
	Fubi::RecognitionResult::Result getRecognitionProgress(std::vector<FubiUser::TrackingData>* userStates, bool restart);

	// Add a state for this combination (used for creating the recognizer)
	void addState(const std::vector<IGestureRecognizer*>& gestureRecognizers, const std::vector<IGestureRecognizer*>& notRecognizers = RecognitionState::s_emptyRecVec,
		double minDuration = 0, double maxDuration = -1, double timeForTransition = 1, double maxInterruption = -1, bool noInterrruptionBeforeMinDuration = false,
		const std::vector<IGestureRecognizer*>& alternativeGestureRecognizers = RecognitionState::s_emptyRecVec, const std::vector<IGestureRecognizer*>& alternativeNotRecognizers = RecognitionState::s_emptyRecVec);
	void addState(const RecognitionState& state);

	// Assign a user to this recognizer (from whom the recognizer retrieves the tracking data for the recognition)
	void setUser(FubiUser* user);

	CombinationRecognizer* clone() const;

	std::string getName() const;

	unsigned int getNumStates() const { return m_RecognitionStates.size(); }

	void setWaitUntilLastStateRecognizersStop(bool enable) { m_waitUntilLastStateRecognizersStop = enable; }

protected:
	bool areAllGesturesRecognized(const RecognitionState& state);


	bool m_running;

	// All recognition states ordered in time
	std::vector<RecognitionState>	m_RecognitionStates;
	// The current state index
	int							m_currentState;
	// When the current state started or the transition started
	double						m_stateStart, m_interruptionStart;
	// If min duration has been reached
	bool						m_minDurationPassed;
	// If the gestures of the current state are temporarly not recognized
	bool						m_interrupted;
	// If the recognition was successful
	bool						m_recognized;
	// User this recognizer is attachded to
	FubiUser*					m_user;
	// gesture id of this recognizer (only predefined ones)
	Fubi::Combinations::Combination	m_gestureID;
	// Name of the recognizer (all recognizers)
	std::string					m_name;

	bool						m_waitUntilLastStateRecognizersStop;

	std::vector<FubiUser::TrackingData> m_userStates;
};