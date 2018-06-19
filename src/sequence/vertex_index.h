//(c) 2016 by Authors
//This file is a part of ABruijn program.
//Released under the BSD license (see LICENSE file)

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <vector>
#include <iostream>

#include <cuckoohash_map.hh>

#include "kmer.h"
#include "sequence_container.h"
#include "../common/config.h"
#include "../common/logger.h"

class VertexIndex
{
public:
	~VertexIndex()
	{
		this->clear();
	}
	VertexIndex(const SequenceContainer& seqContainer, int sampleRate):
		_seqContainer(seqContainer), _outputProgress(false), 
		_sampleRate(sampleRate), _repetitiveFrequency(0)
	{}

	VertexIndex(const VertexIndex&) = delete;
	void operator=(const VertexIndex&) = delete;

private:
	struct ReadPosition
	{
		ReadPosition(FastaRecord::Id readId = FastaRecord::ID_NONE, 
					 int32_t position = 0):
			readId(readId), position(position) {}
		FastaRecord::Id readId;
		int32_t position;
	};

	struct ReadVector
	{
		ReadVector(uint32_t capacity = 0, uint32_t size = 0): 
			capacity(capacity), size(size), data(nullptr) {}
		ReadVector(const ReadPosition& pos):	//construct a vector with single element
			capacity(1), size(1)
		{
			data = new ReadPosition[1];
			data[0] = pos;
		}
		uint32_t capacity;
		uint32_t size;
		ReadPosition* data;
	};

public:
	typedef std::map<size_t, size_t> KmerDistribution;

	class KmerPosIterator
	{
	public:
		KmerPosIterator(ReadVector rv, size_t index, bool revComp, 
						const SequenceContainer& seqContainer):
			rv(rv), index(index), revComp(revComp), 
			seqContainer(seqContainer) 
		{}

		bool operator==(const KmerPosIterator& other) const
		{
			return rv.data == other.rv.data && index == other.index;
		}
		bool operator!=(const KmerPosIterator& other) const
		{
			return !(*this == other);
		}

		ReadPosition operator*() const
		{
			ReadPosition pos = rv.data[index];
			if (revComp)
			{
				pos.readId = pos.readId.rc();
				int32_t seqLen = seqContainer.seqLen(pos.readId);
				pos.position = seqLen - pos.position - 
							   Parameters::get().kmerSize;
			}
			return pos;
		}

		KmerPosIterator& operator++()
		{
			++index;
			return *this;
		}
	
	private:
		ReadVector rv;
		size_t index;
		bool revComp;
		const SequenceContainer& seqContainer;
	};

	class IterHelper
	{
	public:
		IterHelper(ReadVector rv, bool revComp, 
				   const SequenceContainer& seqContainer): 
			rv(rv), revComp(revComp), seqContainer(seqContainer) {}

		KmerPosIterator begin()
		{
			return KmerPosIterator(rv, 0, revComp, seqContainer);
		}

		KmerPosIterator end()
		{
			return KmerPosIterator(rv, rv.size, revComp, seqContainer);
		}

	private:
		ReadVector rv;
		bool revComp;
		const SequenceContainer& seqContainer;
	};

	void countKmers(size_t hardThreshold, int genomeSize);
	void setRepeatCutoff(int minCoverage);
	void buildIndex(int minCoverage);
	void buildIndexUnevenCoverage(int minCoverage);
	void clear();

	IterHelper iterKmerPos(Kmer kmer) const
	{
		bool revComp = kmer.standardForm();
		return IterHelper(_kmerIndex.find(kmer), revComp,
						  _seqContainer);
		//return IterHelper(_lockedIndex->at(kmer), revComp,
		//				  _seqContainer);
	}

	/*struct KmerPosRange
	{
		ReadPosition* begin;
		ReadPosition* end;
		bool rc;
	};
	KmerPosRange iterKmerPos(Kmer kmer) const
	{
		bool revCmp = kmer.standardForm();
		auto rv = _kmerIndex.find(kmer);
		return {rv.data, rv.data + rv.size, revCmp};
	}*/

	bool isSolid(Kmer kmer) const
	{
		kmer.standardForm();
		return _kmerIndex.contains(kmer);
		//return _lockedIndex->count(kmer);
	}

	void outputProgress(bool set) 
	{
		_outputProgress = set;
	}

	const KmerDistribution& getKmerHist() const
	{
		return _kmerDistribution;
	}

	int getSampleRate() const {return _sampleRate;}

private:
	void addFastaSequence(const FastaRecord& fastaRecord);

	const SequenceContainer& _seqContainer;
	KmerDistribution 		 _kmerDistribution;
	bool    _outputProgress;
	int32_t _sampleRate;
	size_t  _repetitiveFrequency;

	const size_t INDEX_CHUNK = 32 * 1024 * 1024 / sizeof(ReadPosition);
	std::vector<ReadPosition*> 		 _memoryChunks;

	cuckoohash_map<Kmer, ReadVector> _kmerIndex;
	cuckoohash_map<Kmer, size_t> 	 _kmerCounts;
	//std::shared_ptr<cuckoohash_map<Kmer, ReadVector>
	//				::locked_table> _lockedIndex;
};
