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

#include "CombinationRecognizer.h"

#include "../Fubi.h"

#include "../FubiConfig.h"

using namespace Fubi;


const std::vector<IGestureRecognizer*> RecognitionState::s_emptyRecVec = std::vector<IGestureRecognizer*>();

CombinationRecognizer::CombinationRecognizer(FubiUser* user, Combinations::Combination gestureID)
	: m_currentState(-1), m_stateStart(0), m_minDurationPassed(false), m_user(user), m_running(false), m_gestureID(gestureID),
	m_interruptionStart(0), m_interrupted(false), m_recognized(false), m_waitUntilLastStateRecognizersStop(false)
{
	m_name = getCombinationName(gestureID);
}

CombinationRecognizer::CombinationRecognizer(const std::string& recognizerName)	// Only for creating a template recognizer, will not work until m_user is set
	: m_currentState(-1), m_stateStart(0), m_minDurationPassed(false), m_user(0x0), m_running(false), m_gestureID(Combinations::NUM_COMBINATIONS),
	m_interruptionStart(0), m_interrupted(false), m_recognized(false), m_name(recognizerName), m_waitUntilLastStateRecognizersStop(false)
{
}

CombinationRecognizer::CombinationRecognizer(const CombinationRecognizer& other)
	: m_currentState(-1), m_stateStart(0), m_minDurationPassed(false), m_user(other.m_user), m_running(false), m_gestureID(other.m_gestureID),
	m_interruptionStart(0), m_interrupted(false), m_recognized(false), m_name(other.m_name), m_waitUntilLastStateRecognizersStop(other.m_waitUntilLastStateRecognizersStop)
{
	for (std::vector<RecognitionState>::const_iterator iter = other.m_RecognitionStates.begin(); iter != other.m_RecognitionStates.end(); ++iter)
	{
		std::vector<IGestureRecognizer*> recognizers;
		for (RecognitionState::GestureIter iter2 = iter->m_gestures.begin(); iter2 != iter->m_gestures.end(); ++iter2)
		{
			recognizers.push_back((*iter2)->clone());
		}
		std::vector<IGestureRecognizer*> notRecognizers;
		for (RecognitionState::GestureIter iter3 = iter->m_notGestures.begin(); iter3 != iter->m_notGestures.end(); ++iter3)
		{
			notRecognizers.push_back((*iter3)->clone());
		}
		std::vector<IGestureRecognizer*> alternativeRecognizers;
		for (RecognitionState::GestureIter iter4 = iter->m_alternativeGestures.begin(); iter4 != iter->m_alternativeGestures.end(); ++iter4)
		{
			alternativeRecognizers.push_back((*iter4)->clone());
		}
		std::vector<IGestureRecognizer*> alternativeNotRecognizers;
		for (RecognitionState::GestureIter iter5 = iter->m_alternativeNotGestures.begin(); iter5 != iter->m_alternativeNotGestures.end(); ++iter5)
		{
			alternativeNotRecognizers.push_back((*iter5)->clone());
		}
		m_RecognitionStates.push_back(RecognitionState(recognizers, notRecognizers, iter->m_minDuration, iter->m_maxDuration, iter->m_timeForTransition, iter->m_maxInterruptionTime, iter->m_noInterrruptionBeforeMinDuration, alternativeRecognizers, alternativeNotRecognizers));
	}
}

CombinationRecognizer::~CombinationRecognizer()
{
	stop();

	for (std::vector<RecognitionState>::iterator iter = m_RecognitionStates.begin(); iter != m_RecognitionStates.end(); ++iter)
	{
		for (RecognitionState::GestureIter iter2 = iter->m_gestures.begin(); iter2 != iter->m_gestures.end(); ++iter2)
		{
			delete (*iter2);
		}
		iter->m_gestures.clear();

		for (RecognitionState::GestureIter iter3 = iter->m_notGestures.begin(); iter3 != iter->m_notGestures.end(); ++iter3)
		{
			delete (*iter3);
		}
		iter->m_alternativeNotGestures.clear();
		for (RecognitionState::GestureIter iter4 = iter->m_alternativeGestures.begin(); iter4 != iter->m_alternativeGestures.end(); ++iter4)
		{
			delete (*iter4);
		}
		iter->m_alternativeGestures.clear();

		for (RecognitionState::GestureIter iter5 = iter->m_alternativeNotGestures.begin(); iter5 != iter->m_alternativeNotGestures.end(); ++iter5)
		{
			delete (*iter5);
		}
		iter->m_alternativeNotGestures.clear();
	}
}

void CombinationRecognizer::start()
{
	if (m_user && !m_running && m_RecognitionStates.size() > 0)
	{
		m_running = true;
		m_currentState = -1;
		m_minDurationPassed = true;
		m_interrupted = false;
		m_recognized = false;
		m_userStates.clear();
	}
}

void CombinationRecognizer::stop()
{
	m_running = false;
	m_currentState = -1;
	m_minDurationPassed = false;
	m_interrupted = false;
}

Fubi::RecognitionResult::Result CombinationRecognizer::getRecognitionProgress(std::vector<FubiUser::TrackingData>* userStates, bool restart)
{
	bool recognized = m_recognized;

	if (recognized)
	{
		if (userStates != 0x0)
		{
			userStates->insert(userStates->begin(), m_userStates.begin(), m_userStates.end());
		}

		if (restart)
			this->restart();
		else
			m_recognized = false;
	}
	else if (isWaitingForLastStateFinish())	// Special case: already recognized, but waiting for the last state to finish because of set flag
		return Fubi::RecognitionResult::WAITING_FOR_LAST_STATE_TO_FINISH;

	return recognized ? Fubi::RecognitionResult::RECOGNIZED : Fubi::RecognitionResult::NOT_RECOGNIZED;
}

bool CombinationRecognizer::areAllGesturesRecognized(const RecognitionState& state)
{
	bool atLeastOneRecognized = false;
	bool atLeastOneFailed = false;
	for (RecognitionState::GestureIter iter = state.m_gestures.begin(); iter != state.m_gestures.end(); ++iter)
	{
		Fubi::RecognitionResult::Result res = (*iter)->recognizeOn(m_user);
		if (res == Fubi::RecognitionResult::NOT_RECOGNIZED 
			|| (res == Fubi::RecognitionResult::TRACKING_ERROR && !(*iter)->m_ignoreOnTrackingError))
		{ 
			atLeastOneRecognized = false;
			atLeastOneFailed = true;
			break;
		}
		else if (res == Fubi::RecognitionResult::RECOGNIZED)
			atLeastOneRecognized = true;
	}
	// Only check not recognizers if not already one recognizer failed
	if (!atLeastOneFailed)
	{
		for (RecognitionState::GestureIter iter = state.m_notGestures.begin(); iter != state.m_notGestures.end(); ++iter)
		{
			// Not recognizers are "recognized" if the recognition fails,
			// and fail if the recognition is successful or cannot be performed because of tracking errors
			Fubi::RecognitionResult::Result res = (*iter)->recognizeOn(m_user);
			if (res == Fubi::RecognitionResult::RECOGNIZED 
				|| (res == Fubi::RecognitionResult::TRACKING_ERROR && !(*iter)->m_ignoreOnTrackingError))
			{ 
				atLeastOneRecognized = false;
				break;
			}
			else if (res == Fubi::RecognitionResult::NOT_RECOGNIZED)
				atLeastOneRecognized = true;
		}
	}
	if (!atLeastOneRecognized)
	{
		atLeastOneFailed = false;
		// No recognition so try alternative recognizers
		for (RecognitionState::GestureIter iter = state.m_alternativeGestures.begin(); iter != state.m_alternativeGestures.end(); ++iter)
		{
			Fubi::RecognitionResult::Result res = (*iter)->recognizeOn(m_user);
			if (res == Fubi::RecognitionResult::NOT_RECOGNIZED 
				|| (res == Fubi::RecognitionResult::TRACKING_ERROR && !(*iter)->m_ignoreOnTrackingError))
			{ 
				atLeastOneRecognized = false;
				atLeastOneFailed = true;
				break;
			}
			else if (res == Fubi::RecognitionResult::RECOGNIZED)
				atLeastOneRecognized = true;
		}
		// Only check not recognizers if not already one recognizer failed
		if (!atLeastOneFailed)
		{
			for (RecognitionState::GestureIter iter = state.m_alternativeNotGestures.begin(); iter != state.m_alternativeNotGestures.end(); ++iter)
			{
				// Not recognizers are "recognized" if the recognition fails,
				// and fail if the recognition is successful or cannot be performed because of tracking errors
				Fubi::RecognitionResult::Result res = (*iter)->recognizeOn(m_user);
				if (res == Fubi::RecognitionResult::RECOGNIZED 
					|| (res == Fubi::RecognitionResult::TRACKING_ERROR && !(*iter)->m_ignoreOnTrackingError))
				{ 
					atLeastOneRecognized = false;
					break;
				}
				else if (res == Fubi::RecognitionResult::NOT_RECOGNIZED)
					atLeastOneRecognized = true;
			}
		}
	}
	return atLeastOneRecognized;
}

void CombinationRecognizer::update()
{
	if (m_running && m_RecognitionStates.size() > 0)
	{
		if (m_minDurationPassed) // Min duration already passed
		{
			// Check for the next state
			bool movedToNextState = false;
			int nextStateID = m_currentState+1;
			if ((unsigned) nextStateID < m_RecognitionStates.size())
			{
				RecognitionState nextState = m_RecognitionStates[nextStateID];			
				if (areAllGesturesRecognized(nextState))
				{
					// Next gestures performed so jump to next state
					m_currentState = nextStateID;
					m_minDurationPassed = false;
					m_interrupted = false;
					m_stateStart = currentTime();
					m_userStates.push_back(m_user->m_currentTrackingData);
					movedToNextState = true;
#ifdef COMBINATIONREC_DEBUG_LOGGING
					Fubi_logDbg("User %d - Combination %s - State %d\n", m_user->m_id, m_name.c_str(), m_currentState);
#endif
				}
			}
			
			if (!movedToNextState && m_currentState > -1)
			{
				if (m_interrupted)
				{
					// Currently interrupted since:
					double interruptionTime = currentTime() - m_interruptionStart;
					bool checkTimeForTransition = m_RecognitionStates[m_currentState].m_timeForTransition >= 0 && m_currentState != m_RecognitionStates.size()-1;
					if (interruptionTime < m_RecognitionStates[m_currentState].m_maxInterruptionTime && areAllGesturesRecognized(m_RecognitionStates[m_currentState]))
					{
						// Continued current state
						m_interrupted = false;
					}
					// The next state has to be reached during time for transition or the current state has to be rekept before max interruption time
					else if ((checkTimeForTransition && interruptionTime > m_RecognitionStates[m_currentState].m_timeForTransition
								&& interruptionTime > m_RecognitionStates[m_currentState].m_maxInterruptionTime)
							|| (!checkTimeForTransition && interruptionTime > m_RecognitionStates[m_currentState].m_maxInterruptionTime))
					{

						if (m_waitUntilLastStateRecognizersStop && m_currentState == m_RecognitionStates.size()-1)
						{
							// Last state finished --> recognized
							m_recognized = true;
							m_userStates.push_back(m_user->m_currentTrackingData);
#ifdef COMBINATIONREC_DEBUG_LOGGING
							Fubi_logDbg("User %d -- Combination %s Recognized!\n", m_user->m_id, m_name.c_str());
#endif
							// Stop the recognition
							stop();
						}
						else
						{
							// Fail, so restart
							restart();
#ifdef COMBINATIONREC_DEBUG_LOGGING
							Fubi_logDbg("User %d - Combination %s - Next gestures too late\n", m_user->m_id, m_name.c_str());
#endif
						}
					}
					// If m_timeForTransition < 0 the user has infinite time for performing the next state
				}
				else if (areAllGesturesRecognized(m_RecognitionStates[m_currentState]))
				{
					double timeInPose = currentTime()- m_stateStart;
					if (m_RecognitionStates[m_currentState].m_maxDuration > 0 && timeInPose > m_RecognitionStates[m_currentState].m_maxDuration)
					{
						if (m_waitUntilLastStateRecognizersStop && m_currentState == m_RecognitionStates.size()-1)
						{
							// Last state finished --> recognized
							m_userStates.push_back(m_user->m_currentTrackingData);
							m_recognized = true;
#ifdef COMBINATIONREC_DEBUG_LOGGING
							Fubi_logDbg("User %d -- Combination %s Recognized!\n", m_user->m_id, m_name.c_str());
#endif
							// Stop the recognition
							stop();
						}
						else
						{
							// Next gestures not performed before max duration is reached so restart recognition
							restart();
#ifdef COMBINATIONREC_DEBUG_LOGGING
							Fubi_logDbg("User %d - Combination %s - Reached max duration\n", m_user->m_id, m_name.c_str());
#endif
						}
					}
				}
				else
				{
					// First frame of interruption
					m_interrupted = true;
					m_interruptionStart = currentTime();
				}
			}
		}
		else if (m_currentState > -1)  // Min duration not yet passed
		{
			bool gesturesRecognized = areAllGesturesRecognized(m_RecognitionStates[m_currentState]);
			double timePast = currentTime() - m_stateStart;
			if (timePast < m_RecognitionStates[m_currentState].m_minDuration) // Within the min duration time frame
			{
				// Check if the current gestures are still fulfilled
				if (!gesturesRecognized)
				{
					if (m_interrupted || m_RecognitionStates[m_currentState].m_noInterrruptionBeforeMinDuration)
					{
						// Interrupted for more than one frame or no interruption allowed at all
						if (m_RecognitionStates[m_currentState].m_noInterrruptionBeforeMinDuration || (currentTime() - m_interruptionStart) > m_RecognitionStates[m_currentState].m_maxInterruptionTime)
						{
#ifdef COMBINATIONREC_DEBUG_LOGGING
							Fubi_logDbg("User %d -- Combination %s - State %d aborted before min duration %.2f\n", m_user->m_id, m_name.c_str(), m_currentState, m_RecognitionStates[m_currentState].m_minDuration);
#endif
							// gestures aborted before min duration is reached
							restart();
						}
					}
					else
					{
						// First frame of interruption
						m_interrupted = true;
						m_interruptionStart = currentTime();
					}
				}
				else
					m_interrupted = false;
			}
			else if (!m_interrupted || gesturesRecognized) // Min duration passed
			{
				// Save user state information for the transition
				m_userStates.push_back(m_user->m_currentTrackingData);

				if (!m_waitUntilLastStateRecognizersStop && m_currentState == m_RecognitionStates.size()-1)
				{
					// Last state finished --> recognized
					m_recognized = true;
#ifdef COMBINATIONREC_DEBUG_LOGGING
					Fubi_logDbg("User %d -- Combination %s Recognized!\n", m_user->m_id, m_name.c_str());
#endif
					// Stop the recognition
					stop();
				}
				else
				{
					// Min duration passed, check for the next state
					m_minDurationPassed = true;
				}
			}
		}
	}
}

void CombinationRecognizer::addState(const std::vector<IGestureRecognizer*>& gestureRecognizers, const std::vector<IGestureRecognizer*>& notRecognizers /*= s_emptyRecVec*/, double minDuration /*= 0*/,
	double maxDuration /*= -1*/, double timeForTransition /*= 1*/, double maxInterruption /*= -1*/, bool noInterrruptionBeforeMinDuration /*= false*/,
	const std::vector<IGestureRecognizer*>& alternativeGestureRecognizers /*= RecognitionState::s_emptyRecVec*/, const std::vector<IGestureRecognizer*>& alternativeNotRecognizers /*= RecognitionState::s_emptyRecVec*/)
{
	if (!gestureRecognizers.empty() || !notRecognizers.empty())
		m_RecognitionStates.push_back(RecognitionState(gestureRecognizers, notRecognizers, minDuration, maxDuration, timeForTransition, maxInterruption, noInterrruptionBeforeMinDuration, alternativeGestureRecognizers, alternativeNotRecognizers));
}

void CombinationRecognizer::addState(const RecognitionState& state)
{
	if (!state.m_gestures.empty() || !state.m_notGestures.empty())
		m_RecognitionStates.push_back(state);
}

CombinationRecognizer* CombinationRecognizer::clone() const
{
	return new CombinationRecognizer(*this);
}

void CombinationRecognizer::setUser(FubiUser* user)
{
	// First stop everything, because the old data won't be usefull with a new user
	stop();
	// Now set the new user
	m_user = user;
}

std::string CombinationRecognizer::getName() const
{
	return m_name;
}