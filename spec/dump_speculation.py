#!env drgn

mitigations = {}

mitigations['spectre_v2_user_stibp'] = prog['spectre_v2_user_stibp']
mitigations['spectre_v2_user_ibpb'] = prog['spectre_v2_user_ibpb']
mitigations['spectre_v2_cmd'] = prog['spectre_v2_cmd']
mitigations['ssb_mode'] = prog['ssb_mode']
mitigations['l1tf_mitigation'] = prog['l1tf_mitigation']
mitigations['srso_mitigation'] = prog['srso_mitigation']
mitigations['srso_cmd'] = prog['srso_cmd']
mitigations['mds_mitigation'] = prog['mds_mitigation']
mitigations['taa_mitigation'] = prog['taa_mitigation']
mitigations['mmio_mitigation'] = prog['mmio_mitigation']
mitigations['srbds_mitigation'] = prog['srbds_mitigation']
mitigations['gds_mitigation'] = prog['gds_mitigation']
mitigations['spectre_v1_mitigation'] = prog['spectre_v1_mitigation']
mitigations['spectre_v2_enabled'] = prog['spectre_v2_enabled']
mitigations['retbleed_mitigation'] = prog['retbleed_mitigation']


for mit in mitigations:
    value = str(mitigations[mit]).split(')')[1]
    
    print(f"{mit:23} \t {value}")
