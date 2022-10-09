/********************************************************************************
* ReactPhysics3D physics library, http://www.reactphysics3d.com                 *
* Copyright (c) 2010-2019 Daniel Chappuis                                       *
*********************************************************************************
*                                                                               *
* This software is provided 'as-is', without any express or implied warranty.   *
* In no event will the authors be held liable for any damages arising from the  *
* use of this software.                                                         *
*                                                                               *
* Permission is granted to anyone to use this software for any purpose,         *
* including commercial applications, and to alter it and redistribute it        *
* freely, subject to the following restrictions:                                *
*                                                                               *
* 1. The origin of this software must not be misrepresented; you must not claim *
*    that you wrote the original software. If you use this software in a        *
*    product, an acknowledgment in the product documentation would be           *
*    appreciated but is not required.                                           *
*                                                                               *
* 2. Altered source versions must be plainly marked as such, and must not be    *
*    misrepresented as being the original software.                             *
*                                                                               *
* 3. This notice may not be removed or altered from any source distribution.    *
*                                                                               *
********************************************************************************/

// If profiling is enabled
#ifdef IS_PROFILING_ACTIVE

// Libraries
#include "Profiler.h"
#include <string>
#include "memory/MemoryManager.h"

using namespace reactphysics3d;

// Constructor
ProfileNode::ProfileNode(const char* name, ProfileNode* parentNode)
    :mName(name), mNbTotalCalls(0), mStartingTime(0), mTotalTime(0),
     mRecursionCounter(0), mParentNode(parentNode), mChildNode(nullptr),
     mSiblingNode(nullptr) {
    reset();
}

// Destructor
ProfileNode::~ProfileNode() {

    delete mChildNode;
    delete mSiblingNode;
}

// Return a pointer to a sub node with a given name
ProfileNode* ProfileNode::findSubNode(const char* name) {

    // Try to find the node among the child nodes
    ProfileNode* child = mChildNode;
    while (child != nullptr) {
        if (child->mName == name) {
            return child;
        }
        child = child->mSiblingNode;
    }

    // The nose has not been found. Therefore, we create it
    // and add it to the profiler tree
    ProfileNode* newNode = new ProfileNode(name, this);
    newNode->mSiblingNode = mChildNode;
    mChildNode = newNode;

    return newNode;
}

// Called when we enter the block of code corresponding to this profile node
void ProfileNode::enterBlockOfCode() {
    mNbTotalCalls++;

    // If the current code is not called recursively
    if (mRecursionCounter == 0) {

        // Get the current system time to initialize the starting time of
        // the profiling of the current block of code
        mStartingTime = Timer::getCurrentSystemTime() * 1000.0L;
    }

    mRecursionCounter++;
}

// Called when we exit the block of code corresponding to this profile node
bool ProfileNode::exitBlockOfCode() {
    mRecursionCounter--;

    if (mRecursionCounter == 0 && mNbTotalCalls != 0) {

        // Get the current system time
        long double currentTime = Timer::getCurrentSystemTime() * 1000.0L;

        // Increase the total elasped time in the current block of code
        mTotalTime += currentTime - mStartingTime;
    }

    // Return true if the current code is not recursing
    return (mRecursionCounter == 0);
}

// Reset the profiling of the node
void ProfileNode::reset() {
    mNbTotalCalls = 0;
    mTotalTime = 0.0L;

    // Reset the child node
    if (mChildNode != nullptr) {
        mChildNode->reset();
    }

    // Reset the sibling node
    if (mSiblingNode != nullptr) {
        mSiblingNode->reset();
    }
}

// Destroy the node
void ProfileNode::destroy() {
    delete mChildNode;
    mChildNode = nullptr;
    delete mSiblingNode;
    mSiblingNode = nullptr;
}

// Constructor
ProfileNodeIterator::ProfileNodeIterator(ProfileNode* startingNode)
    :mCurrentParentNode(startingNode),
      mCurrentChildNode(mCurrentParentNode->getChildNode()){

}

// Enter a given child node
void ProfileNodeIterator::enterChild(int index) {
    mCurrentChildNode = mCurrentParentNode->getChildNode();
    while ((mCurrentChildNode != nullptr) && (index != 0)) {
        index--;
        mCurrentChildNode = mCurrentChildNode->getSiblingNode();
    }

    if (mCurrentChildNode != nullptr) {
        mCurrentParentNode = mCurrentChildNode;
        mCurrentChildNode = mCurrentParentNode->getChildNode();
    }
}

// Enter a given parent node
void ProfileNodeIterator::enterParent() {
    if (mCurrentParentNode->getParentNode() != nullptr) {
        mCurrentParentNode = mCurrentParentNode->getParentNode();
    }
    mCurrentChildNode = mCurrentParentNode->getChildNode();
}

// Constructor
Profiler::Profiler() :mRootNode("Root", nullptr), mDestinations(MemoryManager::getBaseAllocator()) {

	mCurrentNode = &mRootNode;
    mProfilingStartTime = Timer::getCurrentSystemTime() * 1000.0L;
	mFrameCounter = 0;
}

// Destructor
Profiler::~Profiler() {

    removeAllDestinations();

	destroy();
}

// Remove all logs destination previously set
void Profiler::removeAllDestinations() {

    // Delete all the destinations
    for (uint i=0; i<mDestinations.size(); i++) {
        delete mDestinations[i];
    }

    mDestinations.clear();
}

// Method called when we want to start profiling a block of code.
void Profiler::startProfilingBlock(const char* name) {

    // Look for the node in the tree that corresponds to the block of
    // code to profile
    if (name != mCurrentNode->getName()) {
        mCurrentNode = mCurrentNode->findSubNode(name);
    }

    // Start profile the node
    mCurrentNode->enterBlockOfCode();
}

// Method called at the end of the scope where the
// startProfilingBlock() method has been called.
void Profiler::stopProfilingBlock() {

    // Go to the parent node unless if the current block
    // of code is recursing
    if (mCurrentNode->exitBlockOfCode()) {
        mCurrentNode = mCurrentNode->getParentNode();
    }
}

// Reset the timing data of the profiler (but not the profiler tree structure)
void Profiler::reset() {
    mRootNode.reset();
    mRootNode.enterBlockOfCode();
    mFrameCounter = 0;
    mProfilingStartTime = Timer::getCurrentSystemTime() * 1000.0L;
}

// Print the report of the profiler in a given output stream
void Profiler::printReport() {

    // For each destination
    for (auto it = mDestinations.begin(); it != mDestinations.end(); ++it) {

        ProfileNodeIterator* iterator = Profiler::getIterator();

        // Recursively print the report of each node of the profiler tree
        printRecursiveNodeReport(iterator, 0, (*it)->getOutputStream());

        // Destroy the iterator
        destroyIterator(iterator);
    }
}

// Add a file destination to the profiler
void Profiler::addFileDestination(const std::string& filePath, Format format) {

    FileDestination* destination = new FileDestination(filePath, format);
    mDestinations.add(destination);
}

// Add a stream destination to the profiler
void Profiler::addStreamDestination(std::ostream& outputStream, Format format) {

    StreamDestination* destination = new StreamDestination(outputStream, format);
    mDestinations.add(destination);
}

// Recursively print the report of a given node of the profiler tree
void Profiler::printRecursiveNodeReport(ProfileNodeIterator* iterator,
                                        int spacing,
                                        std::ostream& outputStream) {
    iterator->first();

    // If we are at the end of a branch in the profiler tree
    if (iterator->isEnd()) {
        return;
    }

    long double parentTime = iterator->isRoot() ? getElapsedTimeSinceStart() :
                                                  iterator->getCurrentParentTotalTime();
    long double accumulatedTime = 0.0L;
    uint nbFrames = Profiler::getNbFrames();
    for (int i=0; i<spacing; i++) outputStream << " ";
    outputStream << "---------------" << std::endl;
    for (int i=0; i<spacing; i++) outputStream << " ";
    outputStream << "| Profiling : " << iterator->getCurrentParentName() <<
                    " (total running time : " << parentTime << " ms) ---" << std::endl;
    long double totalTime = 0.0L;

    // Recurse over the children of the current node
    int nbChildren = 0;
    for (int i=0; !iterator->isEnd(); i++, iterator->next()) {
        nbChildren++;
        long double currentTotalTime = iterator->getCurrentTotalTime();
        accumulatedTime += currentTotalTime;
        long double fraction = parentTime > std::numeric_limits<long double>::epsilon() ?
                               (currentTotalTime / parentTime) * 100.0L : 0.0L;
        for (int j=0; j<spacing; j++) outputStream << " ";
        outputStream << "|   " << i << " -- " << iterator->getCurrentName() << " : " <<
                        fraction << " % | " << (currentTotalTime / (long double) (nbFrames)) <<
                        " ms/frame (" << iterator->getCurrentNbTotalCalls() << " calls)" <<
                        std::endl;
        totalTime += currentTotalTime;
    }

    if (parentTime < accumulatedTime) {
        outputStream << "Something is wrong !" << std::endl;
    }
    for (int i=0; i<spacing; i++) outputStream << " ";
    long double percentage = parentTime > std::numeric_limits<long double>::epsilon() ?
                ((parentTime - accumulatedTime) / parentTime) * 100.0L : 0.0L;
    long double difference = parentTime - accumulatedTime;
    outputStream << "| Unaccounted : " << difference << " ms (" << percentage << " %)" << std::endl;

    for (int i=0; i<nbChildren; i++){
        iterator->enterChild(i);
        printRecursiveNodeReport(iterator, spacing + 3, outputStream);
        iterator->enterParent();
    }
}

#endif
