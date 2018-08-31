#include <iostream>
#include "dbwrapper.h"
#include <fstream>
#include <boost/iostreams/stream.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/filesystem.hpp>
#include "Log.h"
#include "uint256.h"
#include "random.h"
#include <assert.h>

typedef std::vector<uint8_t> data_chunk;
template <typename Container, typename SourceType, typename CharType>
class container_source
{
public:
	typedef CharType char_type;
	typedef boost::iostreams::source_tag category;

	container_source(const Container& container)
		: container_(container), position_(0)
	{
		static_assert(sizeof(SourceType) == sizeof(CharType), "invalid size");
	}

	std::streamsize read(char_type* buffer, std::streamsize size)
	{
		const auto amount = container_.size() - position_;
		const auto result = std::min(size,
			static_cast<std::streamsize>(amount));

		// TODO: use ios eof symbol (template-based).
		if (result <= 0)
			return -1;

		const auto value = static_cast<typename Container::size_type>(result);
#if 0
		DEBUG_ONLY(const auto maximum =
			std::numeric_limits<typename Container::size_type>::max());
		BITCOIN_ASSERT(value < maximum);
		BITCOIN_ASSERT(position_ + value < maximum);
#endif
		const auto limit = position_ + value;
		const auto start = container_.begin() + position_;

		const auto end = container_.begin() + limit;
		std::copy(start, end, buffer);
		position_ = limit;
		return result;
	}

private:
	const Container& container_;
	typename Container::size_type position_;
};

template <typename Container>
using byte_source = container_source<Container, uint8_t, char>;
using data_source = boost::iostreams::stream<byte_source<data_chunk>>;

typedef boost::iostreams::stream<byte_source<data_chunk>> byte_stream;
template <typename Container, typename SinkType, typename CharType>
class container_sink
{
public:
	typedef CharType char_type;
	typedef boost::iostreams::sink_tag category;

	container_sink(Container& container)
		: container_(container)
	{
		static_assert(sizeof(SinkType) == sizeof(CharType), "invalid size");
	}

	std::streamsize write(const char_type* buffer, std::streamsize size)
	{
		const auto safe_sink = reinterpret_cast<const SinkType*>(buffer);
		container_.insert(container_.end(), safe_sink, safe_sink + size);
		return size;
	}

private:
	Container& container_;
};

template <typename Container>
using byte_sink = container_sink<Container, uint8_t, char>;

using data_sink = boost::iostreams::stream<byte_sink<data_chunk>>;

using namespace boost::filesystem;
using namespace std;

bool is_null_key(const vector<unsigned char>& key) {
	bool isnull = true;

	for (unsigned int i = 0; i < key.size(); i++)
		isnull &= (key[i] == '\x00');

	return isnull;
}
int main()
{
	// We're going to share this path between two wrappers
	path ph = temp_directory_path() / unique_path();
	create_directories(ph);

	// Set up a non-obfuscated wrapper to write some initial data.
	CDBWrapper* dbw = new CDBWrapper(ph, (1 << 10), false, false, false);
	char key = 'k';
	uint256 in = GetRandHash();
	uint256 res;

	assert(dbw->Write(key, in));
	assert(dbw->Read(key, res));
	assert(res.ToString() == in.ToString());

	// Call the destructor to free leveldb LOCK
	delete dbw;

	// Now, set up another wrapper that wants to obfuscate the same directory
	CDBWrapper odbw(ph, (1 << 10), false, false, true);

	// Check that the key/val we wrote with unobfuscated wrapper exists and 
	// is readable.
	uint256 res2;
	assert(odbw.Read(key, res2));
	assert(res2.ToString() == in.ToString());

	assert(!odbw.IsEmpty()); // There should be existing data
	assert(is_null_key(odbw.GetObfuscateKey())); // The key should be an empty string

	uint256 in2 = GetRandHash();
	uint256 res3;

	// Check that we can write successfully
	assert(odbw.Write(key, in2));
	assert(odbw.Read(key, res3));
	assert(res3.ToString() == in2.ToString());
}