/*
 * process_read.cc
 *
 *  Created on: Jun 6, 2016
 *      Author: isovic
 */

#include "owler2/owler2.h"

int Owler2::ProcessRead(const ProgramParameters& parameters, const IndexGappedMinimizer* index, const SingleSequence* read, const std::vector<CompiledShape> &lookup_shapes, OwlerResult *owler_result) {
  const auto &ls = index->get_lookup_shapes();
  std::vector<uint128_t> minimizers;
  minimizers.resize(ls.size() * read->get_sequence_length() * 2);     // Allocate maximum space for the seeds. Factor 2 is for the reverse complement.
  int64_t num_minimizers = 0;
  IndexGappedMinimizer::CollectMinimizers(read->get_data(), read->get_quality(), read->get_sequence_length(), -1, 0, ls, 5, minimizers, &num_minimizers);

  int64_t max_count = ceil(index->get_count_cutoff());

  printf ("max_count = %ld\n", max_count);

//  SeedHashType hash_;
//  hash_.set_empty_key(empty_hash_key);
//  SeedHashValue a;
//  a.start = 123; a.num = 456;
//  hash_[1] = a;
//  a.start = 789; a.num = 012;
//  hash_[2] = a;
//  printf ("hash[1] = (%ld, %ld)\n", hash_[1].start, hash_[1].num);
//  printf ("hash[2] = (%ld, %ld)\n", hash_[2].start, hash_[2].num);
//  printf ("hash[3] = (%ld, %ld)\n", hash_[3].start, hash_[3].num);
//  exit(1);



  int64_t num_valid_hits = 0;
  std::vector<uint128_t> hits;
  hits.resize(read->get_sequence_length());
  int64_t last_hit_id = 0;

  for (int64_t i=0; i<num_minimizers; i++) {
    // Reverse complements are already indexed, so skip them here.
    if (IS_HIT_REVERSE(minimizers[i])) { continue; }

    uint64_t key = GET_KEY_FROM_HIT(minimizers[i]);
    uint64_t pos = GET_REAL_POS_FROM_HIT(minimizers[i]);
    uint128_t coded_pos = minimizers[i] << 64;
    uint128_t *seeds = NULL;
    int64_t num_seeds = 0;
    if (index->GetHits(key, &seeds, &num_seeds)) {
//      printf ("%s skip\n", SeedToString(key, 12).c_str());
      continue;
    }

//    printf ("%X\t%ld\n", key, num_seeds);
    if (max_count > 0 && num_seeds > max_count) {
//      printf ("%s\t%ld\tskip\n", SeedToString(key, 12).c_str(), num_seeds);
      continue;
    }

//    printf ("[%ld] %s\t%ld\t%ld\n", num_valid_hits, SeedToString(key, 12).c_str(), pos, num_seeds);

    if ((last_hit_id + num_seeds) >= hits.size()) {
      hits.resize(hits.size() + read->get_sequence_length());
    }

    memcpy(&hits[last_hit_id], seeds, num_seeds);
    for (int32_t j=0; i<num_seeds; j++) {
      hits[last_hit_id + j] = (hits[last_hit_id + j] & OWLER2_MASK_UPPER_64) | coded_pos;
    }
    last_hit_id += num_seeds;

    num_valid_hits += 1;
  }

  for (int64_t i=0; i<last_hit_id; i++) {
    int64_t qid = GET_SEQ_ID_FROM_HIT(hits[i] >> 64);
    int64_t qstart = GET_POS_FROM_HIT_WITH_REV(hits[i] >> 64);
    int64_t rid = GET_SEQ_ID_FROM_HIT(hits[i]);
    int64_t rstart = GET_POS_FROM_HIT_WITH_REV(hits[i]);
    printf ("qid = %ld, qstart = %ld, rid = %ld, rstart = %ld\n", qid, qstart, rid, rstart);
  }

  printf ("read_length = %ld\n", read->get_sequence_length());

  exit(1);

  return 0;
}
