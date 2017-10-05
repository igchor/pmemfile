/*
 * Copyright 2017, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * offset_mapping.cpp -- unit test for offset mapping tree
 */
#include "pmemfile_test.hpp"
#include "offset_mapping.h"
#include "blocks.h"

class basic : public pmemfile_test {
public:
	basic() : pmemfile_test()
	{
	}
};

TEST_F(offset_mapping, add_level)
{
	PMEMfile *pfp = pmemfile_open(pfp, "/file1", PMEMFILE_O_CREAT |
						PMEMFILE_O_EXCL | PMEMFILE_O_RDWR,
						0644);

	struct offset_map *map = offset_map_new(pfp);
	struct pmemfile_block_desc b1, b2, b3, b4;
	uint64_t expected_range_length = MIN_BLOCK_SIZE;

	ASSERT_EQ(map->range_length, expected_range_length);

	b1.size = MIN_BLOCK_SIZE;
	b1.offset = 0;
	insert_block(map, &block1);

	expected_range_length <<= N_CHILDREN_POW;
	ASSERT_EQ(map->range_length, expected_range_length);

	b2.size = MIN_BLOCK_SIZE;
	b2.offset = N_CHILDREN * MIN_BLOCK_SIZE;
	insert_block(map, &b2);

	expected_range_length <<= N_CHILDREN_POW;
	ASSERT_EQ(map->range_length, expected_range_length);

	b3.size = MIN_BLOCK_SIZE;
	b3.offset = (N_CHILDREN * N_CHILDREN + 1) * MIN_BLOCK_SIZE;
	insert_block(map, &b3);

	expected_range_length <<= (2 * N_CHILDREN_POW);
	ASSERT_EQ(map->range_length, expected_range_length);
}

TEST_F(offset_mapping, cleanup)
{
	PMEMfile *pfp = pmemfile_open(pfp, "/file1", PMEMFILE_O_CREAT |
						PMEMFILE_O_EXCL | PMEMFILE_O_RDWR,
						0644);

	struct offset_map *map = offset_map_new(pfp);
	struct pmemfile_block_desc b1, b2, b3, b4;

	b1.size = MIN_BLOCK_SIZE;
	b1.offset = 0;
	insert_block(map, &b1);

	b2.size = MIN_BLOCK_SIZE;
	b2.offset = MIN_BLOCK_SIZE;
	insert_block(map, &b2);

	b3.size = MIN_BLOCK_SIZE * N_CHILDREN;
	b3.offset = MIN_BLOCK_SIZE * N_CHILDREN;
	insert_block(map, &b3);

	b4.size = MIN_BLOCK_SIZE * N_CHILDREN;
	b4.offset = MIN_BLOCK_SIZE * (N_CHILDREN + 2);
	insert_block(map, &b4);

	ASSERT_EQ(map->range_length,
			  MIN_BLOCK_SIZE << (N_CHILDREN_POW * 2));

	remove_block(map, &b3);

	ASSERT_EQ(map->range_length,
			  MIN_BLOCK_SIZE << (N_CHILDREN_POW * 2));

	remove_block(map, &b4);

	ASSERT_EQ(map->range_length,
			  MIN_BLOCK_SIZE << N_CHILDREN_POW);
	ASSERT_EQ(map->entry.data.children[0], b1);
	ASSERT_EQ(map->entry.data.children[1], b2);
}

TEST_F(offset_mapping, find_block)
{
	PMEMfile *pfp = pmemfile_open(pfp, "/file1", PMEMFILE_O_CREAT |
						PMEMFILE_O_EXCL | PMEMFILE_O_RDWR,
						0644);

	struct offset_map *map = offset_map_new(pfp);
	struct pmemfile_block_desc b1, b2, b3, b4;

	b1.size = N_CHILDREN * MIN_BLOCK_SIZE;
	b1.offset = 3 * MIN_BLOCK_SIZE;
	insert_block(map, &b1);

	b1.prev = blockp_as_oid(&b2);

	b2.size = MIN_BLOCK_SIZE;
	b2.offset = 0;
	insert_block(map, &b2);

	for(size_t offset = b1.offset; offset < b1.offset + b1.size;
			offset += MIN_BLOCK_SIZE) {
		ASSERT_EQ(block_find_closest(map, offset), b1);
	}

	ASSERT_EQ(block_find_closest(map,
			 (N_CHILDREN - 1) * MIN_BLOCK_SIZE), b1);
	ASSERT_EQ(block_find_closest(map,
			 b1.offset - MIN_BLOCK_SIZE), b2);
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		fprintf(stderr, "usage: %s global_path", argv[0]);
		exit(1);
	}

	global_path = argv[1];

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
