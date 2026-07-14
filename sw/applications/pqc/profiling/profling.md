# Profling Results (ML-KEM)

Totals used for percentages:
- Keygen: 900000
- Encaps: 965000
- Decaps: 1108000

## Keygen (lines 3-17)

| Operation | Cycles | % | Operation | Cycles | % |
|---|---:|---:|---|---:|---:|
| indcpa_keypair_derand hash_g | 25316 | 2.8129% | indcpa_keypair_derand gen_a | 326863 | 36.3181% |
| indcpa_keypair_derand poly_getnoise_eta1(skpv) | 109342 | 12.1491% | indcpa_keypair_derand poly_getnoise_eta1(e) | 109342 | 12.1491% |
| indcpa_keypair_derand polyvec_ntt(skpv) | 45878 | 5.0976% | indcpa_keypair_derand polyvec_ntt(e) | 45878 | 5.0976% |
| indcpa_keypair_derand matvec+tomont | 55291 | 6.1434% | indcpa_keypair_derand polyvec_add | 4933 | 0.5481% |
| indcpa_keypair_derand polyvec_reduce | 5945 | 0.6606% | indcpa_keypair_derand pack_sk | 6463 | 0.7181% |
| indcpa_keypair_derand pack_pk | 6766 | 0.7518% | crypto_kem_keypair_derand indcpa_keypair_derand | 177799 | 19.7554% |
| crypto_kem_keypair_derand memcpy pk->sk | 7261 | 0.8068% | crypto_kem_keypair_derand hash_h | 149183 | 16.5759% |
| crypto_kem_keypair_derand memcpy z | 300 | 0.0333% |  |  |  |

## Encaps (lines 19-40)

| Operation | Cycles | % | Operation | Cycles | % |
|---|---:|---:|---|---:|---:|
| crypto_kem_enc_derand memcpy coins->buf | 300 | 0.0311% | crypto_kem_enc_derand hash_h | 149182 | 15.4593% |
| crypto_kem_enc_derand hash_g | 25804 | 2.6740% | indcpa_enc unpack_pk | 4987 | 0.5168% |
| indcpa_enc poly_frommsg | 3295 | 0.3415% | indcpa_enc gen_at | 328997 | 34.0930% |
| indcpa_enc poly_getnoise_eta1(sp) | 109958 | 11.3946% | indcpa_enc poly_getnoise_eta2(ep) | 61472 | 6.3702% |
| indcpa_enc poly_getnoise_eta2(epp) | 30736 | 3.1851% | indcpa_enc polyvec_ntt(sp) | 45877 | 4.7541% |
| indcpa_enc matvec b | 48113 | 4.9858% | indcpa_enc polyvec_basemul_acc_montgomery(v) | 24057 | 2.4930% |
| indcpa_enc polyvec_invntt_tomont(b) | 63122 | 6.5411% | indcpa_enc poly_invntt_tomont(v) | 31563 | 3.2708% |
| indcpa_enc polyvec_add(b,ep) | 4936 | 0.5115% | indcpa_enc poly_add(v,epp) | 2341 | 0.2426% |
| indcpa_enc poly_add(v,k) | 2338 | 0.2423% | indcpa_enc polyvec_reduce(b) | 6183 | 0.6407% |
| indcpa_enc poly_reduce(v) | 2855 | 0.2959% | indcpa_enc pack_ciphertext | 18705 | 1.9383% |
| crypto_kem_enc_derand indcpa_enc | 181702 | 18.8292% | crypto_kem_enc_derand memcpy kr->ss | 303 | 0.0314% |

## Decaps (lines 42-73)

| Operation | Cycles | % | Operation | Cycles | % |
|---|---:|---:|---|---:|---:|
| indcpa_dec unpack_ciphertext | 12006 | 1.0836% | indcpa_dec unpack_sk | 4690 | 0.4233% |
| indcpa_dec polyvec_ntt(b) | 45909 | 4.1434% | indcpa_dec polyvec_basemul_acc_montgomery(mp) | 24043 | 2.1699% |
| indcpa_dec poly_invntt_tomont(mp) | 31603 | 2.8523% | indcpa_dec poly_sub | 2339 | 0.2111% |
| indcpa_dec poly_reduce(mp) | 2855 | 0.2577% | indcpa_dec poly_tomsg | 4038 | 0.3644% |
| crypto_kem_dec indcpa_dec | 143402 | 12.9424% | crypto_kem_dec memcpy z->buf | 303 | 0.0273% |
| crypto_kem_dec hash_g | 26128 | 2.3581% | indcpa_enc unpack_pk | 4985 | 0.4499% |
| indcpa_enc poly_frommsg | 3294 | 0.2973% | indcpa_enc gen_at | 326826 | 29.4969% |
| indcpa_enc poly_getnoise_eta1(sp) | 109346 | 9.8688% | indcpa_enc poly_getnoise_eta2(ep) | 61108 | 5.5152% |
| indcpa_enc poly_getnoise_eta2(epp) | 30554 | 2.7576% | indcpa_enc polyvec_ntt(sp) | 45939 | 4.1461% |
| indcpa_enc matvec b | 48115 | 4.3425% | indcpa_enc polyvec_basemul_acc_montgomery(v) | 24058 | 2.1713% |
| indcpa_enc polyvec_invntt_tomont(b) | 63236 | 5.7072% | indcpa_enc poly_invntt_tomont(v) | 31620 | 2.8538% |
| indcpa_enc polyvec_add(b,ep) | 4937 | 0.4456% | indcpa_enc poly_add(v,epp) | 2341 | 0.2113% |
| indcpa_enc poly_add(v,k) | 2338 | 0.2110% | indcpa_enc polyvec_reduce(b) | 6183 | 0.5580% |
| indcpa_enc poly_reduce(v) | 2855 | 0.2577% | indcpa_enc pack_ciphertext | 18801 | 1.6968% |
| crypto_kem_dec indcpa_enc(cmp) | 181703 | 16.3992% | crypto_kem_dec verify | 7710 | 0.6958% |
| crypto_kem_dec rkprf | 153828 | 13.8834% | crypto_kem_dec cmov | 362 | 0.0327% |


# Profling Results (ML-DSA)

Totals used for percentages:
- Keygen: 3444000
- Sign: 6820000
- Verify: 3618000

## ML-DSA Keygen

| Operation | Cycles | % | Operation | Cycles | % |
|---|---:|---:|---|---:|---:|
| crypto_sign_keypair memcpy(seedbuf) | 298 | 0.0087% | crypto_sign_keypair shake256(seed expansion) | 26765 | 0.7771% |
| crypto_sign_keypair polyvec_matrix_expand | 2109205 | 61.2429% | crypto_sign_keypair polyvecl_uniform_eta | 175816 | 5.1050% |
| crypto_sign_keypair polyveck_uniform_eta | 200468 | 5.8208% | crypto_sign_keypair polyvecl_ntt(s1hat) | 194000 | 5.6330% |
| crypto_sign_keypair polyvec_matrix_pointwise_montgomery | 188245 | 5.4659% | crypto_sign_keypair polyveck_reduce(t1) | 10326 | 0.2998% |
| crypto_sign_keypair polyveck_invntt_tomont(t1) | 221327 | 6.4265% | crypto_sign_keypair polyveck_add(t1,s2) | 9367 | 0.2720% |
| crypto_sign_keypair polyveck_caddq | 9296 | 0.2699% | crypto_sign_keypair polyveck_power2round | 12506 | 0.3631% |
| crypto_sign_keypair pack_pk | 6776 | 0.1967% | crypto_sign_keypair shake256(tr) | 245773 | 7.1363% |
| crypto_sign_keypair pack_sk | 21428 | 0.6222% |  |  |  |

## ML-DSA Sign

| Operation | Cycles | % | Operation | Cycles | % |
|---|---:|---:|---|---:|---:|
| crypto_sign copy message into sm | 304 | 0.0045% | crypto_sign_signature prepare pre | 6 | 0.0001% |
| crypto_sign_signature_internal unpack_sk | 25824 | 0.3787% | crypto_sign_signature_internal compute mu | 27123 | 0.3977% |
| crypto_sign_signature_internal compute rhoprime | 27763 | 0.4071% | crypto_sign_signature_internal polyvec_matrix_expand | 2107945 | 30.9083% |
| crypto_sign_signature_internal polyvecl_ntt(s1) | 193999 | 2.8446% | crypto_sign_signature_internal polyveck_ntt(s2) | 193998 | 2.8445% |
| crypto_sign_signature_internal polyveck_ntt(t0) | 193994 | 2.8445% | crypto_sign_signature_internal polyvecl_uniform_gamma1 | 519285 | 7.6141% |
| crypto_sign_signature_internal polyvecl_ntt(z) | 194000 | 2.8446% | crypto_sign_signature_internal matrix pointwise | 188255 | 2.7603% |
| crypto_sign_signature_internal polyveck_reduce(w1) | 10326 | 0.1514% | crypto_sign_signature_internal polyveck_invntt_tomont(w1) | 221195 | 3.2433% |
| crypto_sign_signature_internal polyveck_caddq(w1) | 9296 | 0.1363% | crypto_sign_signature_internal polyveck_decompose | 24734 | 0.3627% |
| crypto_sign_signature_internal polyveck_pack_w1 | 8269 | 0.1212% | crypto_sign_signature_internal challenge hash | 176914 | 2.5940% |
| crypto_sign_signature_internal poly_challenge | 33902 | 0.4971% | crypto_sign_signature_internal poly_ntt(cp) | 48501 | 0.7112% |
| crypto_sign_signature_internal pointwise(z,cp,s1) | 38054 | 0.5580% | crypto_sign_signature_internal polyvecl_invntt_tomont(z) | 221194 | 3.2433% |
| crypto_sign_signature_internal polyvecl_add(z,y) | 9312 | 0.1365% | crypto_sign_signature_internal polyvecl_reduce(z) | 10390 | 0.1523% |
| crypto_sign_signature_internal polyvecl_chknorm(reject) | 5258 | 0.0771% | crypto_sign_signature_internal polyvecl_uniform_gamma1 | 519285 | 7.6141% |
| crypto_sign_signature_internal polyvecl_ntt(z) | 194000 | 2.8446% | crypto_sign_signature_internal matrix pointwise | 188255 | 2.7603% |
| crypto_sign_signature_internal polyveck_reduce(w1) | 10326 | 0.1514% | crypto_sign_signature_internal polyveck_invntt_tomont(w1) | 221195 | 3.2433% |
| crypto_sign_signature_internal polyveck_caddq(w1) | 9296 | 0.1363% | crypto_sign_signature_internal polyveck_decompose | 24734 | 0.3627% |
| crypto_sign_signature_internal polyveck_pack_w1 | 8269 | 0.1212% | crypto_sign_signature_internal challenge hash | 176914 | 2.5940% |
| crypto_sign_signature_internal poly_challenge | 33881 | 0.4968% | crypto_sign_signature_internal poly_ntt(cp) | 48501 | 0.7112% |
| crypto_sign_signature_internal pointwise(z,cp,s1) | 38054 | 0.5580% | crypto_sign_signature_internal polyvecl_invntt_tomont(z) | 221194 | 3.2433% |
| crypto_sign_signature_internal polyvecl_add(z,y) | 9312 | 0.1365% | crypto_sign_signature_internal polyvecl_reduce(z) | 10390 | 0.1523% |
| crypto_sign_signature_internal polyvecl_chknorm(pass) | 10287 | 0.1508% | crypto_sign_signature_internal pointwise(h,cp,s2) | 38056 | 0.5580% |
| crypto_sign_signature_internal polyveck_invntt_tomont(h) | 221195 | 3.2433% | crypto_sign_signature_internal polyveck_sub(w0,h) | 9371 | 0.1374% |
| crypto_sign_signature_internal polyveck_reduce(w0) | 10321 | 0.1513% | crypto_sign_signature_internal polyveck_chknorm(w0,pass) | 11372 | 0.1667% |
| crypto_sign_signature_internal pointwise(h,cp,t0) | 38053 | 0.5580% | crypto_sign_signature_internal polyveck_invntt_tomont(h2) | 221195 | 3.2433% |
| crypto_sign_signature_internal polyveck_reduce(h) | 10394 | 0.1524% | crypto_sign_signature_internal polyveck_chknorm(h,pass) | 11372 | 0.1667% |
| crypto_sign_signature_internal polyveck_add(w0,h) | 9306 | 0.1365% | crypto_sign_signature_internal polyveck_make_hint | 15379 | 0.2255% |
| crypto_sign_signature_internal pack_sig | 20218 | 0.2965% | crypto_sign_signature internal call | 234452 | 3.4377% |
| crypto_sign crypto_sign_signature | 437224 | 6.4109% | crypto_sign update smlen | 7 | 0.0001% |

## ML-DSA Verify

| Operation | Cycles | % | Operation | Cycles | % |
|---|---:|---:|---|---:|---:|
| crypto_sign_open compute mlen | 8 | 0.0002% | crypto_sign_verify prepare pre | 6 | 0.0002% |
| crypto_sign_verify_internal unpack_pk | 7675 | 0.2121% | crypto_sign_verify_internal unpack_sig(pass) | 43291 | 1.1965% |
| crypto_sign_verify_internal polyvecl_chknorm(pass) | 11375 | 0.3144% | crypto_sign_verify_internal compute mu | 274374 | 7.5836% |
| crypto_sign_verify_internal poly_challenge | 33873 | 0.9362% | crypto_sign_verify_internal polyvec_matrix_expand | 2093033 | 57.8506% |
| crypto_sign_verify_internal polyvecl_ntt(z) | 194002 | 5.3621% | crypto_sign_verify_internal matrix pointwise | 188247 | 5.2031% |
| crypto_sign_verify_internal poly_ntt(cp) | 48500 | 1.3405% | crypto_sign_verify_internal polyveck_shiftl(t1) | 7250 | 0.2004% |
| crypto_sign_verify_internal polyveck_ntt(t1) | 194002 | 5.3621% | crypto_sign_verify_internal pointwise(t1,cp,t1) | 37017 | 1.0231% |
| crypto_sign_verify_internal polyveck_sub(w1,t1) | 9375 | 0.2591% | crypto_sign_verify_internal polyveck_reduce(w1) | 10388 | 0.2871% |
| crypto_sign_verify_internal polyveck_invntt_tomont(w1) | 221194 | 6.1137% | crypto_sign_verify_internal polyveck_caddq(w1) | 9363 | 0.2588% |
| crypto_sign_verify_internal polyveck_use_hint | 19012 | 0.5255% | crypto_sign_verify_internal polyveck_pack_w1 | 8269 | 0.2286% |
| crypto_sign_verify_internal challenge recompute | 175668 | 4.8554% | crypto_sign_verify_internal challenge compare(pass) | 269 | 0.0074% |
| crypto_sign_verify internal call | 253990 | 7.0202% | crypto_sign_open crypto_sign_verify(pass) | 444658 | 12.2902% |
| crypto_sign_open copy message | 401 | 0.0111% |  |  |  |

