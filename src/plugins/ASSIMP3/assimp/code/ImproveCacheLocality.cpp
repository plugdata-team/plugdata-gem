/*
---------------------------------------------------------------------------
Open Asset Import Library (assimp)
---------------------------------------------------------------------------

Copyright (c) 2006-2012, assimp team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the assimp team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the assimp team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------
*/

/** @file Implementation of the post processing step to improve the cache locality of a mesh.
 * <br>
 * The algorithm is roughly basing on this paper:
 * http://www.cs.princeton.edu/gfx/pubs/Sander_2007_%3ETR/tipsy.pdf
 *   .. although overdraw rduction isn't implemented yet ...
 */

#include "AssimpPCH.h"

// internal headers
#include "ImproveCacheLocality.h"
#include "VertexTriangleAdjacency.h"

using namespace Assimp;

// ------------------------------------------------------------------------------------------------
// Constructor to be privately used by Importer
ImproveCacheLocalityProcess::ImproveCacheLocalityProcess() {
	configCacheDepth = PP_ICL_PTCACHE_SIZE;
}

// ------------------------------------------------------------------------------------------------
// Destructor, private as well
ImproveCacheLocalityProcess::~ImproveCacheLocalityProcess()
{
	// nothing to do here
}

// ------------------------------------------------------------------------------------------------
// Returns whether the processing step is present in the given flag field.
bool ImproveCacheLocalityProcess::IsActive( unsigned int pFlags) const
{
	return (pFlags & aiProcess_ImproveCacheLocality) != 0;
}

// ------------------------------------------------------------------------------------------------
// Setup configuration
void ImproveCacheLocalityProcess::SetupProperties(const Importer* pImp)
{
	// AI_CONFIG_PP_ICL_PTCACHE_SIZE controls the target cache size for the optimizer
	configCacheDepth = pImp->GetPropertyInteger(AI_CONFIG_PP_ICL_PTCACHE_SIZE,PP_ICL_PTCACHE_SIZE);
}

// ------------------------------------------------------------------------------------------------
// Executes the post processing step on the given imported data.
void ImproveCacheLocalityProcess::Execute( aiScene* pScene)
{
	if (!pScene->mNumMeshes) {
		DefaultLogger::get()->debug("ImproveCacheLocalityProcess skipped; there are no meshes");
		return;
	}

	DefaultLogger::get()->debug("ImproveCacheLocalityProcess begin");

	float out = 0.f;
	unsigned int numf = 0, numm = 0;
	for( unsigned int a = 0; a < pScene->mNumMeshes; a++){
		const float res = ProcessMesh( pScene->mMeshes[a],a);
		if (res) {
			numf += pScene->mMeshes[a]->mNumFaces;
			out  += res;
			++numm;
		}
	}
	if (!DefaultLogger::isNullLogger()) {
		char szBuff[128]; // should be sufficiently large in every case
		::sprintf(szBuff,"Cache relevant are %i meshes (%i faces). Average output ACMR is %f",
			numm,numf,out/numf);

		DefaultLogger::get()->info(szBuff);
		DefaultLogger::get()->debug("ImproveCacheLocalityProcess finished. ");
	}
}

// ------------------------------------------------------------------------------------------------
// Improves the cache coherency of a specific mesh
float ImproveCacheLocalityProcess::ProcessMesh( aiMesh* pMesh, unsigned int meshNum)
{
	// TODO: rewrite this to use std::vector or boost::shared_array
	ai_assert(NULL != pMesh);

	// Check whether the input data is valid
	// - there must be vertices and faces 
	// - all faces must be triangulated or we can't operate on them
	if (!pMesh->HasFaces() || !pMesh->HasPositions())
		return 0.f;

	if (pMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)	{
		DefaultLogger::get()->error("This algorithm works on triangle meshes only");
		return 0.f;
	}

	if(pMesh->mNumVertices <= configCacheDepth) {
		return 0.f;
	}

	float fACMR = 3.f;
	const aiFace* const pcEnd = pMesh->mFaces+pMesh->mNumFaces;

	// Input ACMR is for logging purposes only
	if (!DefaultLogger::isNullLogger())		{

		unsigned int* piFIFOStack = new unsigned int[configCacheDepth];
		memset(piFIFOStack,0xff,configCacheDepth*sizeof(unsigned int));
		unsigned int* piCur = piFIFOStack;
		const unsigned int* const piCurEnd = piFIFOStack + configCacheDepth;

		// count the number of cache misses
		unsigned int iCacheMisses = 0;
		for (const aiFace* pcFace = pMesh->mFaces;pcFace != pcEnd;++pcFace)	{

			for (unsigned int qq = 0; qq < 3;++qq) {
				bool bInCache = false;

				for (unsigned int* pp = piFIFOStack;pp < piCurEnd;++pp)	{
					if (*pp == pcFace->mIndices[qq])	{
						// the vertex is in cache
						bInCache = true;
						break;
					}
				}
				if (!bInCache)	{
					++iCacheMisses;
					if (piCurEnd == piCur) {
						piCur = piFIFOStack;
					}
					*piCur++ = pcFace->mIndices[qq];
				}
			}
		}
		delete[] piFIFOStack;
		fACMR = (float)iCacheMisses / pMesh->mNumFaces;
		if (3.0 == fACMR)	{
			char szBuff[128]; // should be sufficiently large in every case

			// the JoinIdenticalVertices process has not been executed on this
			// mesh, otherwise this value would normally be at least minimally
			// smaller than 3.0 ...
			sprintf(szBuff,"Mesh %i: Not suitable for vcache optimization",meshNum);
			DefaultLogger::get()->warn(szBuff);
			return 0.f;
		}
	}

	// first we need to build a vertex-triangle adjacency list
	VertexTriangleAdjacency adj(pMesh->mFaces,pMesh->mNumFaces, pMesh->mNumVertices,true);

	// build a list to store per-vertex caching time stamps
	unsigned int* const piCachingStamps = new unsigned int[pMesh->mNumVertices];
	memset(piCachingStamps,0x0,pMesh->mNumVertices*sizeof(unsigned int));

	// allocate an empty output index buffer. We store the output indices in one large array.
	// Since the number of triangles won't change the input faces can be reused. This is how 
	// we save thousands of redundant mini allocations for aiFace::mIndices
	const unsigned int iIdxCnt = pMesh->mNumFaces*3;
	unsigned int* const piIBOutput = new unsigned int[iIdxCnt];
	unsigned int* piCSIter = piIBOutput;

	// allocate the flag array to hold the information
	// whether a face has already been emitted or not
	std::vector<bool> abEmitted(pMesh->mNumFaces,false);

	// dead-end vertex index stack
	std::stack<unsigned int, std::vector<unsigned int> > sDeadEndVStack;

	// create a copy of the piNumTriPtr buffer
	unsigned int* const piNumTriPtr = adj.mLiveTriangles;
	const std::vector<unsigned int> piNumTriPtrNoModify(piNumTriPtr, piNumTriPtr + pMesh->mNumVertices);
	
	// get the largest number of referenced triangles and allocate the "candidate buffer"
	unsigned int iMaxRefTris = 0; {
		const unsigned int* piCur = adj.mLiveTriangles;
		const unsigned int* const piCurEnd = adj.mLiveTriangles+pMesh->mNumVertices;
		for (;piCur != piCurEnd;++piCur) {
			iMaxRefTris = std::max(iMaxRefTris,*piCur);
		}
	}
	unsigned int* piCandidates = new unsigned int[iMaxRefTris*3];
	unsigned int iCacheMisses = 0;

	// ...................................................................................
	/** PSEUDOCODE for the algorithm

		A = Build-Adjacency(I) Vertex-triangle adjacency
		L = Get-Triangle-Counts(A) Per-vertex live triangle counts
		C = Zero(Vertex-Count(I)) Per-vertex caching time stamps
		D = Empty-Stack() Dead-end vertex stack
		E = False(Triangle-Count(I)) Per triangle emitted flag
		O = Empty-Index-Buffer() Empty output buffer
		f = 0 Arbitrary starting vertex
		s = k+1, i = 1 Time stamp and cursor
		while f >= 0 For all valid fanning vertices
			N = Empty-Set() 1-ring of next candidates
			for each Triangle t in Neighbors(A, f)
				if !Emitted(E,t)
					for each Vertex v in t
						Append(O,v) Output vertex
						Push(D,v) Add to dead-end stack
						Insert(N,v) Register as candidate
						L[v] = L[v]-1 Decrease live triangle count
						if s-C[v] > k If not in cache
							C[v] = s Set time stamp
							s = s+1 Increment time stamp
					E[t] = true Flag triangle as emitted
			Select next fanning vertex
			f = Get-Next-Vertex(I,i,k,N,C,s,L,D)
		return O
		*/
	// ...................................................................................

	int ivdx = 0;
	int ics = 1;
	int iStampCnt = configCacheDepth+1;
	while (ivdx >= 0)	{

		unsigned int icnt = piNumTriPtrNoModify[ivdx]; 
		unsigned int* piList = adj.GetAdjacentTriangles(ivdx);
		unsigned int* piCurCandidate = piCandidates;

		// get all triangles in the neighborhood
		for (unsigned int tri = 0; tri < icnt;++tri)	{

			// if they have not yet been emitted, add them to the output IB
			const unsigned int fidx = *piList++;
			if (!abEmitted[fidx])	{

				// so iterate through all vertices of the current triangle
				const aiFace* pcFace = &pMesh->mFaces[ fidx ];
				for (unsigned int* p = pcFace->mIndices, *p2 = pcFace->mIndices+3;p != p2;++p)	{
					const unsigned int dp = *p;

					// the current vertex won't have any free triangles after this step
					if (ivdx != (int)dp) {
						// append the vertex to the dead-end stack
						sDeadEndVStack.push(dp);

						// register as candidate for the next step
						*piCurCandidate++ = dp;

						// decrease the per-vertex triangle counts
						piNumTriPtr[dp]--;
					}

					// append the vertex to the output index buffer
					*piCSIter++ = dp;

					// if the vertex is not yet in cache, set its cache count
					if (iStampCnt-piCachingStamps[dp] > configCacheDepth) {
						piCachingStamps[dp] = iStampCnt++;
						++iCacheMisses;
					}
				}
				// flag triangle as emitted
				abEmitted[fidx] = true;
			}
		}

		// the vertex has now no living adjacent triangles anymore
		piNumTriPtr[ivdx] = 0;

		// get next fanning vertex
		ivdx = -1; 
		int max_priority = -1;
		for (unsigned int* piCur = piCandidates;piCur != piCurCandidate;++piCur)	{
      const unsigned int dp = *piCur;

			// must have live triangles
			if (piNumTriPtr[dp] > 0)	{
				int priority = 0;

				// will the vertex be in cache, even after fanning occurs?
				unsigned int tmp;
				if ((tmp = iStampCnt-piCachingStamps[dp]) + 2*piNumTriPtr[dp] <= configCacheDepth) {
					priority = tmp;
				}

				// keep best candidate
				if (priority > max_priority) {
					max_priority = priority;
					ivdx = dp;
				}
			}
		}
		// did we reach a dead end?
		if (-1 == ivdx)	{
			// need to get a non-local vertex for which we have a good chance that it is still 
			// in the cache ...
			while (!sDeadEndVStack.empty())	{
				unsigned int iCachedIdx = sDeadEndVStack.top();
				sDeadEndVStack.pop();
				if (piNumTriPtr[ iCachedIdx ] > 0)	{
					ivdx = iCachedIdx;
					break;
				}
			}

			if (-1 == ivdx)	{
				// well, there isn't such a vertex. Simply get the next vertex in input order and
				// hope it is not too bad ...
				while (ics < (int)pMesh->mNumVertices)	{
					++ics;
					if (piNumTriPtr[ics] > 0)	{
						ivdx = ics;
						break;
					}
				}
			}
		}
	}
	float fACMR2 = 0.0f;
	if (!DefaultLogger::isNullLogger()) {
		fACMR2 = (float)iCacheMisses / pMesh->mNumFaces;

		// very intense verbose logging ... prepare for much text if there are many meshes
		if ( DefaultLogger::get()->getLogSeverity() == Logger::VERBOSE) {
			char szBuff[128]; // should be sufficiently large in every case

			::sprintf(szBuff,"Mesh %i | ACMR in: %f out: %f | ~%.1f%%",meshNum,fACMR,fACMR2,
				((fACMR - fACMR2) / fACMR) * 100.f);
			DefaultLogger::get()->debug(szBuff);
		}

		fACMR2 *= pMesh->mNumFaces;
	}
	// sort the output index buffer back to the input array
	piCSIter = piIBOutput;
	for (aiFace* pcFace = pMesh->mFaces; pcFace != pcEnd;++pcFace)	{
		pcFace->mIndices[0] = *piCSIter++;
		pcFace->mIndices[1] = *piCSIter++;
		pcFace->mIndices[2] = *piCSIter++;
	}

	// delete temporary storage
	delete[] piCachingStamps;
	delete[] piIBOutput;
	delete[] piCandidates;

	return fACMR2;
}
