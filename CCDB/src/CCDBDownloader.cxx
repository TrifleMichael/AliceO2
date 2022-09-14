#include <CCDB/CCDBDownloader.h>

#include <curl/curl.h>
#include <unordered_map>
#include <stdio.h>
#include <stdlib.h>
#include <uv.h>
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <thread>     // get_id
#include <vector>
#include <condition_variable>
#include <mutex>

#include <chrono>   // time measurement
#include <unistd.h> // time measurement

#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>

// THIS IS THE SSL TEST ! THIS IS THE SSL TEST ! THIS IS THE SSL TEST ! THIS IS THE SSL TEST !
bool aliceServer = true;
// std::string alicePathsCS = "https://alice-ccdb.cern.ch/TRD/Calib/DCSDPsU/1659949587465,https://alice-ccdb.cern.ch/GLO/Config/GRPMagField/1659621342493,https://alice-ccdb.cern.ch/MFT/Config/Params/1659947087892,https://alice-ccdb.cern.ch/TPC/Calib/VDriftTgl/1,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Gaintbl_Uniform_FGAN0_2011-01/1,https://alice-ccdb.cern.ch/PHS/BadMap/Chi/1,https://alice-ccdb.cern.ch/ITS/Calib/Confdbmap/1451606461000,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Krypton_2015-01/1,https://alice-ccdb.cern.ch/PHS/Calib/CalibParams/1651356383449,https://alice-ccdb.cern.ch/PHS/Calib/Pedestals/1657807103014,https://alice-ccdb.cern.ch/TOF/Config/DCSDPconfig/1652733375051,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Krypton_2011-02/1,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Gaintbl_Uniform_FGAN0_2012-01/1,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Krypton_2015-02/1,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Krypton_2011-03/1,https://alice-ccdb.cern.ch/TPC/Calib/PadGainFull/0,https://alice-ccdb.cern.ch/MFT/Config/AlpideParam/1657663439000,https://alice-ccdb.cern.ch/ZDC/Calib/RecoConfigZDC/1546300800000,https://alice-ccdb.cern.ch/EMC/Config/DCSDPconfig/1647423729718,https://alice-ccdb.cern.ch/TPC/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/ZDC/Calib/InterCalibConfig/1546300800000,https://alice-ccdb.cern.ch/PHS/BadMap/1,https://alice-ccdb.cern.ch/ZDC/Calib/TowerCalib/1546300800000,https://alice-ccdb.cern.ch/GLO/Config/Collimators/1659949276728,https://alice-ccdb.cern.ch/MFT/Condition/DCSDPs/1659949674726,https://alice-ccdb.cern.ch/TPC/Calib/Gas/1659949565413,https://alice-ccdb.cern.ch/GLO/Config/EnvVars/1659949276728,https://alice-ccdb.cern.ch/CTP/Calib/OrbitReset/1659938355618,https://alice-ccdb.cern.ch/CTP/Calib/Scalers/1659946806420,https://alice-ccdb.cern.ch/TRD/Calib/DCSDPsI/1659949587465,https://alice-ccdb.cern.ch/FT0/Calib/DCSDPs/1659949426082,https://alice-ccdb.cern.ch/CPV/PedestalRun/ChannelEfficiencies/1659935186741,https://alice-ccdb.cern.ch/TPC/Calib/PedestalNoise/1659935701974,https://alice-ccdb.cern.ch/CPV/Calib/Pedestals/1659935186741,https://alice-ccdb.cern.ch/CTP/Config/Config/1659946806420,https://alice-ccdb.cern.ch/TPC/Calib/Pulser/1659936185889,https://alice-ccdb.cern.ch/EMC/Calib/Temperature/1659949599544,https://alice-ccdb.cern.ch/TPC/Calib/TimeGain/0,https://alice-ccdb.cern.ch/EMC/Calib/FeeDCS/1659690910837,https://alice-ccdb.cern.ch/FT0/Calib/ChannelTimeOffset/1546300800000,https://alice-ccdb.cern.ch/GLO/Config/LHCIFDataPoints/1659949276728,https://alice-ccdb.cern.ch/TOF/Calib/DCSDPs/1659949388995,https://alice-ccdb.cern.ch/GLO/Config/GRPLHCIF/1659937041924,https://alice-ccdb.cern.ch/GLO/Config/GRPECS/1659887718188,https://alice-ccdb.cern.ch/GLO/GRP/BunchFilling/1657583876199,https://alice-ccdb.cern.ch/GLO/Config/Geometry/1546300800000,https://alice-ccdb.cern.ch/GLO/Param/MatLUT/1636019021295,https://alice-ccdb.cern.ch/CPV/PedestalRun/HighPedChannels/1659935186741,https://alice-ccdb.cern.ch/TPC/Calib/HV/1659949565413,https://alice-ccdb.cern.ch/EMC/Align/1638479347757,https://alice-ccdb.cern.ch/FT0/Calib/CTFDictionary/1655282522000,https://alice-ccdb.cern.ch/TOF/Calib/LHCphase/0,https://alice-ccdb.cern.ch/EMC/Calib/Align/1,https://alice-ccdb.cern.ch/CPV/PedestalRun/DeadChannels/1659935186741,https://alice-ccdb.cern.ch/TRD/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/TRD/Calib/ChamberStatus/1,https://alice-ccdb.cern.ch/EMC/Calib/TimeCalibParams/1626472048000,https://alice-ccdb.cern.ch/GLO/Calib/MeanVertex/1635257044785,https://alice-ccdb.cern.ch/EMC/Config/CalibParam/1658945928000,https://alice-ccdb.cern.ch/TRD/Calib/DCSDPsGas/1659949080207,https://alice-ccdb.cern.ch/TPC/Calib/Temperature/1659949565413,https://alice-ccdb.cern.ch/CPV/Calib/BadChannelMap/1659935743007,https://alice-ccdb.cern.ch/PHS/Config/RecoParams/1657915200001,https://alice-ccdb.cern.ch/MFT/Calib/DeadMap/0,https://alice-ccdb.cern.ch/ZDC/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/FV0/Config/DCSDPconfig/1,https://alice-ccdb.cern.ch/TOF/Calib/ChannelCalib/1657153698848,https://alice-ccdb.cern.ch/PHS/BadMap/Ped/1,https://alice-ccdb.cern.ch/TRD/Calib/PadNoise/1,https://alice-ccdb.cern.ch/PHS/Align/1638479347757,https://alice-ccdb.cern.ch/TRD/Calib/CalVdriftExB/1,https://alice-ccdb.cern.ch/ZDC/Calib/TDCCorr/1546300800000,https://alice-ccdb.cern.ch/ZDC/Calib/EnergyCalib/1546300800000,https://alice-ccdb.cern.ch/MFT/Calib/NoiseMap/1659890496672,https://alice-ccdb.cern.ch/ITS/Calib/CTFDictionary/1653192000000,https://alice-ccdb.cern.ch/ZDC/Calib/Align/1,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Krypton_2018-01/1,https://alice-ccdb.cern.ch/TPC/Calib/Align/1,https://alice-ccdb.cern.ch/MFT/Config/ClustererParam/1657726413855,https://alice-ccdb.cern.ch/PHS/Calib/Align/1,https://alice-ccdb.cern.ch/ZDC/Config/Module/1546300800000,https://alice-ccdb.cern.ch/TPC/Config/FEEPad/0,https://alice-ccdb.cern.ch/ZDC/Calib/IntegrationParam/1635570656996,https://alice-ccdb.cern.ch/TOF/Calib/Align/1,https://alice-ccdb.cern.ch/TRD/Calib/PadStatus/1,https://alice-ccdb.cern.ch/PHS/Calib/BadMap/1,https://alice-ccdb.cern.ch/ITS/Calib/ClusterDictionary/1653192000000,https://alice-ccdb.cern.ch/TRK/Calib/Align/1,https://alice-ccdb.cern.ch/MID/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/ITS/Calib/DeadMap/0,https://alice-ccdb.cern.ch/ITS/Calib/NoiseMap/0,https://alice-ccdb.cern.ch/ITS/Config/ClustererParam/1657726378777,https://alice-ccdb.cern.ch/TOF/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/TRK/Align/1638479347757,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Krypton_2012-01/1,https://alice-ccdb.cern.ch/ITS/Calib/Align/1640991600000,https://alice-ccdb.cern.ch/MCH/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/PHS/BadMap/Occ/1,https://alice-ccdb.cern.ch/MFT/Config/DCSDPconfig/1653494821018,https://alice-ccdb.cern.ch/MCH/Calib/Align/1,https://alice-ccdb.cern.ch/IT3/Calib/Align/1,https://alice-ccdb.cern.ch/ZDC/Calib/TDCCalib/1546300800000,https://alice-ccdb.cern.ch/ZDC/Calib/BaselineCalib/1546300800000,https://alice-ccdb.cern.ch/ZDC/Align/1638479347757,https://alice-ccdb.cern.ch/TOF/Align/1638479347757,https://alice-ccdb.cern.ch/MID/Config/DCSDPconfig/1653499219078,https://alice-ccdb.cern.ch/MID/Align/1638479347757,https://alice-ccdb.cern.ch/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5n-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5549/1,https://alice-ccdb.cern.ch/ITS/Align/1635322620830,https://alice-ccdb.cern.ch/TST/Calib/Align/1,https://alice-ccdb.cern.ch/TPC/Calib/TopologyGain/0,https://alice-ccdb.cern.ch/CPV/PhysicsRun/GainCalibData/946684800000,https://alice-ccdb.cern.ch/HMP/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/CTP/Calib/CTFDictionary/1626472048000,https://alice-ccdb.cern.ch/TPC/Config/DCSDPconfig/1650631890013,https://alice-ccdb.cern.ch/IT3/Align/1638479347757,https://alice-ccdb.cern.ch/CTP/Calib/Align/1,https://alice-ccdb.cern.ch/TOF/Calib/FEELIGHT/1659947098000,https://alice-ccdb.cern.ch/HMP/Calib/Align/1,https://alice-ccdb.cern.ch/TOF/Calib/HVStatus/1659621342493,https://alice-ccdb.cern.ch/ITS/Config/AlpideParam/1657442962000,https://alice-ccdb.cern.ch/TPC/Calib/CorrectionMap/1,https://alice-ccdb.cern.ch/TRD/Calib/Align/1,https://alice-ccdb.cern.ch/ZDC/Config/Sim/1546300800000,https://alice-ccdb.cern.ch/MID/Calib/BadChannels/1659890536060,https://alice-ccdb.cern.ch/MFT/Calib/Align/1635322620830,https://alice-ccdb.cern.ch/MCH/Calib/BadChannel/1659935281422,https://alice-ccdb.cern.ch/TPC/Calib/LaserTracks/1659513086496,https://alice-ccdb.cern.ch/CPV/Calib/CTFDictionary/1653192000000,https://alice-ccdb.cern.ch/TRD/Config/LinkToHCIDMapping/1577833200000,https://alice-ccdb.cern.ch/FDD/Align/1638479347757,https://alice-ccdb.cern.ch/PHS/Calib/CTFDictionary/1653192000000,https://alice-ccdb.cern.ch/MID/Calib/Align/1,https://alice-ccdb.cern.ch/TRD/Calib/ChamberCalibrations/1,https://alice-ccdb.cern.ch/TPC/Calib/PadGainResidual/0,https://alice-ccdb.cern.ch/MFT/Align/1635322620830,https://alice-ccdb.cern.ch/MCH/Align/1638479347757,https://alice-ccdb.cern.ch/EMC/Calib/CTFDictionary/1653192000000,https://alice-ccdb.cern.ch/TRD/Align/1638479347757,https://alice-ccdb.cern.ch/CPV/Calib/Align/1,https://alice-ccdb.cern.ch/FV0/Config/LookupTable/1651241791741,https://alice-ccdb.cern.ch/TRD/Calib/LocalGainFactor/1,https://alice-ccdb.cern.ch/CPV/PedestalRun/FEEThresholds/1659935186741,https://alice-ccdb.cern.ch/MFT/Calib/CTFDictionary/1653192000000,https://alice-ccdb.cern.ch/ITS/DCS_CONFIG/1659532359719,https://alice-ccdb.cern.ch/TRD/Config/DCSDPconfig/1654615142813,https://alice-ccdb.cern.ch/CPV/Calib/Gains/1627542202422,https://alice-ccdb.cern.ch/FDD/Calib/Align/1,https://alice-ccdb.cern.ch/ACO/Align/1638479347757,https://alice-ccdb.cern.ch/TRD/Calib/NoiseMapMCM/1,https://alice-ccdb.cern.ch/TRD/Calib/CalT0/1,https://alice-ccdb.cern.ch/FV0/Calib/Align/1640991600000,https://alice-ccdb.cern.ch/MFT/Calib/ClusterDictionary/1653192000000,https://alice-ccdb.cern.ch/FV0/Calibration/ChannelTimeOffset/1546300800000,https://alice-ccdb.cern.ch/CPV/Align/1638479347757,https://alice-ccdb.cern.ch/TRD/Calib/HalfChamberStatusQC/1577833200000,https://alice-ccdb.cern.ch/GRP/Config/DCSDPconfig/1651753934367,https://alice-ccdb.cern.ch/MCH/BadChannelCalib/1652347115925,https://alice-ccdb.cern.ch/GLO/Config/GeometryAligned/1640991600000,https://alice-ccdb.cern.ch/FCT/Calib/Align/1,https://alice-ccdb.cern.ch/FV0/Calib/CTFDictionary/1655282522000,https://alice-ccdb.cern.ch/CPV/Config/CPVSimParams/946684800000,https://alice-ccdb.cern.ch/HMP/Align/1638479347757,https://alice-ccdb.cern.ch/EMC/Temperature/2254,https://alice-ccdb.cern.ch/GRP/StartOrbit/0,https://alice-ccdb.cern.ch/FV0/Calib/ChannelTimeOffset/1546300800000,https://alice-ccdb.cern.ch/CTP/Config/TriggerOffsets/1,https://alice-ccdb.cern.ch/EMC/Config/ChannelScaleFactors/1546300800000,https://alice-ccdb.cern.ch/FDD/Config/LookupTable/1654105278523,https://alice-ccdb.cern.ch/TOF/Calib/Diagnostic/1,https://alice-ccdb.cern.ch/TOF/Calib/LVStatus/1659621343526,https://alice-ccdb.cern.ch/FT3/Calib/Align/1,https://alice-ccdb.cern.ch/EMC/FeeDCS/1653646593677,https://alice-ccdb.cern.ch/TPC/Align/1638479347757,https://alice-ccdb.cern.ch/FT0/Calib/Align/1640991600000,https://alice-ccdb.cern.ch/FDD/Config/DCSDPconfig/1,https://alice-ccdb.cern.ch/MCH/Config/DCSDPconfig/1653499164150,https://alice-ccdb.cern.ch/FV0/Align/1638479347757,https://alice-ccdb.cern.ch/EMC/Calib/BadChannelMap/1626472048000,https://alice-ccdb.cern.ch/TPC/Calib/CE/1659890954106,https://alice-ccdb.cern.ch/TRD/OnlineGainTables/Krypton_2011-01/1,https://alice-ccdb.cern.ch/FT0/Align/1638479347757,https://alice-ccdb.cern.ch/CPV/Config/CPVCalibParams/1653639365626,https://alice-ccdb.cern.ch/FDD/Calib/CTFDictionary/1655282522000,https://alice-ccdb.cern.ch/FT3/Align/1638479347757,https://alice-ccdb.cern.ch/TRD/Calib/DCSDPs/1653646795136,https://alice-ccdb.cern.ch/FT0/Config/DCSDPconfig/1,https://alice-ccdb.cern.ch/FT0/Calibration/ChannelTimeOffset/1546300800000,https://alice-ccdb.cern.ch/FT0/Config/LookupTable/1654105278523";
// std::string aliceEtagsCS = "a9b7bce3-16f9-11ed-8002-0aa1229c1b9a,218d3bd3-13fd-11ed-8002-0aa1229c1b9a,90b44c90-16f3-11ed-8002-0aa1229c1b9a,5497bcc3-0843-11ed-8000-200114580202,68862530-5488-11ec-975c-2a01cb15032a,73f9080e-b1d5-11ec-a6f9-0aa202a21b9a,79c59697-cbc4-11ec-b790-200114580202,668fd370-5488-11ec-975c-2a01cb15032a,2a114c11-c8d2-11ec-8000-0aa2041a1b9a,42397504-037d-11ed-8001-0aa204131b9a,d4cc38a5-d557-11ec-8000-0aa1229c1b9a,64fb29b0-5488-11ec-975c-2a01cb15032a,68e75800-5488-11ec-975c-2a01cb15032a,674fc810-5488-11ec-975c-2a01cb15032a,656eac00-5488-11ec-975c-2a01cb15032a,b929213c-a46e-11ec-982e-0aa2043e1b9a,2f375991-023d-11ed-8000-2a010e0a0b16,abbc3456-cca5-11ec-b790-200114580d00,598f0ccf-a50d-11ec-982e-0aa1229c1b9a,e2e145c0-515f-11ec-975c-2a010e0a0b16,6cfda43e-cca5-11ec-b790-200114580d00,4fe5e667-9b14-11ec-982e-0aa202a21b9a,0ef2f88b-cca6-11ec-b790-200114580d00,0eb1ed91-16fa-11ed-8002-0aa1229c1b9a,96942d4c-16f9-11ed-8002-0aa1229c1b9a,08bc0570-16fa-11ed-8002-0aa1229c1b9a,0eab9fc9-16fa-11ed-8002-0aa1229c1b9a,3c58dd90-16df-11ed-8002-0aa140641b9a,db92a6bd-16f5-11ed-8002-0aa140651b9a,a9b2f465-16f9-11ed-8002-0aa1229c1b9a,67bcba6d-16fa-11ed-8002-0aa1229c1b9a,db2e561a-16d7-11ed-8002-0aa202a21b9a,0e47ab39-16d9-11ed-8002-0aa204151b9a,db3e6115-16d7-11ed-8002-0aa202a21b9a,4e8fb968-16f4-11ed-8002-0aa140651b9a,2eb7b048-16da-11ed-8002-0aa204151b9a,69c80f1a-16f9-11ed-8002-0aa1229c1b9a,fc6eb7f0-a46e-11ec-982e-0aa2043e1b9a,1b4b931d-149f-11ed-8002-0aa1229c1b9a,756d3f8b-d5cd-11ec-9222-200114580d00,00ed5e2a-16fa-11ed-8002-0aa1229c1b9a,51969d3e-16fa-11ed-8002-0aa1229c1b9a,2cd79ee5-16dc-11ed-8002-0aa1229c1b9a,36ccfe21-172b-11ed-8002-0aa202fc1b9a,48f3aa8f-0175-11ed-8000-0aa2040a1b9a,f50a6d64-2879-11ed-8141-200114580202,b9bd65e0-3d53-11ec-8357-2a010e0a09fb,db326e41-16d7-11ed-8002-0aa202a21b9a,08c06681-16fa-11ed-8002-0aa1229c1b9a,37ac8c90-52eb-11ec-8357-2a010e0a0b16,06548824-ec98-11ec-bea3-2a010e0a0b16,33982998-52a8-11ec-bf8f-0aa2041d1b9a,f8ad3060-bc3e-11ec-b66d-2a010e0a0b16,db366855-16d7-11ed-8002-0aa202a21b9a,e0ebde60-515f-11ec-975c-2a010e0a0b16,e724d29a-d22d-11ec-8dd8-511cc1ec24ee,ebed4af0-a944-11ec-975c-25fc52e124ee,9c582d06-2c77-11ed-ac0b-2a010e0a0b16,162e7e03-1a3a-11ed-b286-200114580202,49d9b221-16fa-11ed-8002-0aa1229c1b9a,08c47e86-16fa-11ed-8002-0aa1229c1b9a,26aa11b2-16d9-11ed-8002-0aa202a21b9a,f96ba879-0613-11ed-8002-0aa202a21b9a,4eca84a0-a047-11ec-8357-25fc52e1250c,c8bc7660-515f-11ec-8357-2a010e0a0b16,a43b9d43-0170-11ed-a336-2a010e0a0b16,b43c321e-2d1b-11ed-8727-200114580202,73f992a0-b1d5-11ec-a6f9-0aa202a21b9a,e81c639a-d22d-11ec-8dd8-511cc1ec24ee,3760dd90-52eb-11ec-8357-2a010e0a0b16,e6e40961-d22d-11ec-8dd8-511cc1ec24ee,0d5c2790-cca6-11ec-b790-200114580d00,73c62b2f-cca4-11ec-981d-200114580d00,ce825c19-166f-11ed-8002-0aa200321b9a,1d67e544-d92e-11ec-9e90-2a010e0a0b16,f95bf513-bc3e-11ec-b66d-2a010e0a0b16,680eab40-5488-11ec-975c-2a01cb15032a,f7fe8760-bc3e-11ec-b66d-2a010e0a0b16,280dd2ac-02c1-11ed-8000-200114580202,f86688b4-bc3e-11ec-b66d-2a010e0a0b16,c4613b4a-0439-11ed-8000-200114580202,dcf98353-a46e-11ec-982e-0aa2043e1b9a,81042ff0-5468-11ec-975c-2a010e0a0b16,f843ba4b-bc3e-11ec-b66d-2a010e0a0b16,e8418bc7-d22d-11ec-8dd8-511cc1ec24ee,a5b6f9bf-9b39-11ec-982e-0aa202a21b9a,e1b7711f-d92a-11ec-8eb6-200114580202,fa2ad4cb-bc3e-11ec-b66d-2a010e0a0b16,d7c41e60-515f-11ec-975c-2a010e0a0b16,57a0a230-a047-11ec-8357-25fc52e1250c,5cc82830-9fae-11ec-8357-25fc52e1250c,13286527-02c1-11ed-8000-200114580202,d98e3000-515f-11ec-975c-2a010e0a0b16,393157d0-52eb-11ec-8357-2a010e0a0b16,65f1beb0-5488-11ec-975c-2a01cb15032a,31ee4ae3-2ebb-11ed-ac0b-200114580202,d5e52530-515f-11ec-975c-2a010e0a0b16,73f8583d-b1d5-11ec-a6f9-0aa202a21b9a,b6208708-dc44-11ec-8eb6-2a010e0a0b16,f9160ba9-bc3e-11ec-b66d-2a010e0a0b16,fa07d9ff-bc3e-11ec-b66d-2a010e0a0b16,0bc5c80f-cca6-11ec-b790-200114580d00,c6fe667e-f23b-11ec-b9c0-2a010e0a0b16,385e2950-52eb-11ec-8357-2a010e0a0b16,373b7b40-52eb-11ec-8357-2a010e0a0b16,f3a71542-dc4e-11ec-9e90-2a010e0a0b16,383b8620-52eb-11ec-8357-2a010e0a0b16,e6c280c9-d22d-11ec-8dd8-511cc1ec24ee,5516eb40-a543-11ec-975c-25fc52e124ee,f9e54f0f-bc3e-11ec-b66d-2a010e0a0b16,27c55ffd-aaa4-11ec-982e-0aa2043e1b9a,c3df1e31-e67c-11ec-8003-0aa202a21b9a,e6656370-515f-11ec-975c-2a010e0a0b16,f75d650f-c805-11ec-b790-2a010e0a0b16,ee1b51f7-c23a-11ec-8000-0aa2049b1b9a,390e8d90-52eb-11ec-8357-2a010e0a0b16,8c2ed1ff-ccc0-11ec-b790-2a010e0a0b16,992dd7f5-16f3-11ed-8002-0aa1229c1b9a,f8d05560-bc3e-11ec-b66d-2a010e0a0b16,215b3c15-13fd-11ed-8002-0aa1229c1b9a,5f2dc7de-003f-11ed-a97d-2a010e0a0b16,aa40ff05-3171-11ed-ac0b-2a010e0a0b16,f82105ae-bc3e-11ec-b66d-2a010e0a0b16,4bd9e0c3-ccc2-11ec-b790-2a010e0a0b16,e5b9bee8-166f-11ed-8002-0aa204151b9a,a974ae51-bc3f-11ec-b66d-2a010e0a0b16,1a17f296-16d8-11ed-8002-0aa200321b9a,13bad351-1301-11ed-8002-0aa204e61b9a,74f5dcc0-d92f-11ec-9e90-2a010e0a0b16,c4a1ab5e-2ec9-11ed-ac0b-200114580202,38c79980-52eb-11ec-8357-2a010e0a0b16,70196a15-d940-11ec-9e90-2a010e0a0b16,f938e7ba-bc3e-11ec-b66d-2a010e0a0b16,e704b36a-d22d-11ec-8dd8-511cc1ec24ee,02d318c8-a46f-11ec-982e-0aa2043e1b9a,30795be0-a3c4-11ec-975c-25fc52e124ee,38181fa0-52eb-11ec-8357-2a010e0a0b16,4904c8f8-d92f-11ec-9e90-2a010e0a0b16,3712e4a0-52eb-11ec-8357-2a010e0a0b16,f88944a8-bc3e-11ec-b66d-2a010e0a0b16,f82fe729-c7c6-11ec-981d-2a010e0a0b16,e7a02548-d22d-11ec-8dd8-511cc1ec24ee,db3a4589-16d7-11ed-8002-0aa202a21b9a,0511e1a6-d92e-11ec-9e90-2a010e0a0b16,f3820402-132d-11ed-8002-0aa1229c1b9a,29cf3933-e675-11ec-9e90-200114580202,fdbfdde0-5441-11ec-8357-200114580202,f9c530b8-bc3e-11ec-b66d-2a010e0a0b16,38ea15a0-52eb-11ec-8357-2a010e0a0b16,e871e7e5-d22d-11ec-8dd8-511cc1ec24ee,4edf2888-2ec7-11ed-8727-200114580202,38865750-e310-11ec-9e90-2a010e0a0b16,d9d33094-d92a-11ec-8eb6-200114580202,b54b6ad5-d832-11ec-9e90-200114580202,37872a40-52eb-11ec-8357-2a010e0a0b16,50eca180-f22b-11ec-b9c0-200114580202,64acb793-cc6f-11ec-981d-200114580202,cb6d771c-d1d4-11ec-8000-0aa200321b9a,6607d43d-2ebb-11ed-8727-200114580202,37d89e41-d043-11ec-981d-2a010e0a0b16,120b9920-ec98-11ec-bea3-2a010e0a0b16,5ce9988b-d60c-11ec-8000-0aa202a21b9a,37ceba90-52eb-11ec-8357-2a010e0a0b16,9f3f7140-dda7-11ec-8000-0aa1229c1b9a,7cb85e90-9e2c-11ec-8357-90ce809b250c,ea2bed9b-d832-11ec-8eb6-200114580202,907486a2-ecaa-11ec-bea3-2a010e0a0b16,27dec102-13e7-11ed-8000-200114580d00,975c780e-ee4d-11ec-841e-200114580202,19889fe2-b96e-11ec-b66d-200114580202,10a14683-17ce-11ed-8002-0aa1229c1b9a,fa4d6144-bc3e-11ec-b66d-2a010e0a0b16,16ac6d3b-dda6-11ec-8000-0aa1229c1b9a,36eebad0-52eb-11ec-8357-2a010e0a0b16,3f67485b-e310-11ec-9e90-2a010e0a0b16,9be86b16-0170-11ed-a336-2a010e0a0b16,d2f69665-dc4e-11ec-9e90-2a010e0a0b16,38a32190-52eb-11ec-8357-2a010e0a0b16,cdca72f0-a944-11ec-975c-25fc52e124ee,de66bf44-1670-11ed-8002-0aa2040e1b9a,649059f0-5488-11ec-975c-2a01cb15032a,3880cc80-52eb-11ec-8357-2a010e0a0b16,41410990-dd95-11ec-8000-0aa202a21b9a,f4683693-ec97-11ec-bea3-2a010e0a0b16,39542210-52eb-11ec-8357-2a010e0a0b16,8d982d22-dda6-11ec-8000-0aa1229c1b9a,386b34f9-fc71-11ec-a336-200114580204,0e34e534-d5cd-11ec-90c5-200114580d00,a86e62a9-ee4d-11ec-841e-200114580202";
// std::string ccdbEtagsCS = "4fe98e00-712f-11eb-82cf-0aa1402f250c,3cb3dfd8-9893-11ec-8d5e-0aa14014250c,2b0b0f00-1c4c-11ec-8bdc-200114580204,3cb4e5a7-9893-11ec-8d5e-0aa14014250c,2b9a7dc0-1c4c-11ec-8bdc-200114580204,4b62d089-d21c-11ec-894c-c0a80209250c,d7e3ffd0-712e-11eb-828b-0aa1402f250c,99d48ff0-05a5-11ec-a10f-200114580204,2b675fd0-1c4c-11ec-8bdc-200114580204,d9cfa330-712e-11eb-82ce-0aa1402f250c,3cb5b0e4-9893-11ec-8d5e-0aa14014250c,ce8bc31d-bc3e-11ec-9002-c0a80209250c,9ced7ab8-9893-11ec-8d5e-0aa14015250c,0087d650-711b-11eb-996f-200114580202,d7eb2bc0-712e-11eb-82a9-0aa1402f250c,106111a1-e897-11ec-8f1d-c0a80209250c,d7ec1620-712e-11eb-82a9-0aa1402f250c,da68fd00-712e-11eb-82ce-0aa1402f250c,11fac573-febb-11ec-a565-c0a80209250c,2b2dd940-1c4c-11ec-8bdc-200114580204,d7ef4a70-712e-11eb-82be-0aa1402f250c,a86d5475-a50f-11ec-82c1-0aa1229c250c,9ceaf470-9893-11ec-8d5e-0aa14015250c,48e12533-a065-11ec-9f44-808d13fc250c,d7dc1090-712e-11eb-8282-0aa1402f250c,d7ecd970-712e-11eb-82ae-0aa1402f250c,2afbccc0-1c4c-11ec-8bdc-200114580204,e019d73f-a39a-11ec-800e-869e1ccf250c,d7ef9890-712e-11eb-82c2-0aa1402f250c,d7e58670-712e-11eb-8296-0aa1402f250c,d7ef9890-712e-11eb-82c3-0aa1402f250c,d7e4c320-712e-11eb-828f-0aa1402f250c,3f444131-a458-11ec-81db-808d14b6250c,d80bac10-712e-11eb-82ce-0aa1402f250c,4b5d3497-d21c-11ec-894c-c0a80209250c,2b954da0-1c4c-11ec-8bdc-200114580204,2b101810-1c4c-11ec-8bdc-200114580204,2bb88d10-1c4c-11ec-8bdc-200114580204,2b1e21d0-1c4c-11ec-8bdc-200114580204,d7ef7180-712e-11eb-82c0-0aa1402f250c,2b05b7d0-1c4c-11ec-8bdc-200114580204,5cb545da-dabf-11ec-bea2-c0a80209250c,008bf500-711b-11eb-996f-200114580202,d98839f0-712e-11eb-82ce-0aa1402f250c,d7f8e760-712e-11eb-82ce-0aa1402f250c,f6eb7c80-21e8-11ec-a1bc-808d13fc250c,d7e6bef0-712e-11eb-829a-0aa1402f250c,e6daf742-9893-11ec-8d5e-0aa14014250c,d7eefc50-712e-11eb-82bb-0aa1402f250c,657decc0-b1ae-11ec-9d4d-c0a80209250c,e25956d1-c169-11ec-82c1-c0a80209250c,4e2ab4e2-d4af-11ec-8d4a-c0a80209250c,473d5994-13f5-11ed-a09e-c0a80209250c,6d4bfe90-1c49-11ec-8bd2-200114580204,ca25540b-5455-11ec-a337-200114580204,cd7761b1-1486-11ed-a0f2-c0a80209250c,d7ef2360-712e-11eb-82bb-0aa1402f250c,e6dbfbde-9893-11ec-8d5e-0aa14014250c,e6dd24b4-9893-11ec-8d5e-0aa14014250c,e6d93534-9893-11ec-8d5e-0aa14014250c,d7f2a5d0-712e-11eb-82cc-0aa1402f250c,c89d711c-5455-11ec-a337-200114580204,d7ef7180-712e-11eb-82c2-0aa1402f250c,2b9fd4f0-1c4c-11ec-8bdc-200114580204,777f31d0-05a5-11ec-a10a-200114580204,2af6eac0-1c4c-11ec-8bdc-200114580204,4b6190e8-d21c-11ec-894c-c0a80209250c,6bf191a8-cfa7-11ec-a067-c0a80209250c,d7e4ea30-712e-11eb-828f-0aa1402f250c,d7e58670-712e-11eb-8297-0aa1402f250c,2ab41560-1c4c-11ec-8bdc-200114580204,d7f564f0-712e-11eb-82ce-0aa1402f250c,a5b974de-bbf1-11ec-8e56-c0a80209250c,ceca30b9-bc3e-11ec-9002-c0a80209250c,3f44b099-a458-11ec-81db-808d14b6250c,d7e5d490-712e-11eb-8299-0aa1402f250c,d7e38aa0-712e-11eb-828a-0aa1402f250c,f5ba315d-1672-11ed-a5ad-c0a80209250c,079f32e3-d21c-11ec-894c-c0a80209250c,2b232ae0-1c4c-11ec-8bdc-200114580204,9cec9482-9893-11ec-8d5e-0aa14015250c,ac5fb7f8-5f60-11ec-9a09-808d5c3b250c,e5defba8-eebc-11ec-8d02-c0a80209250c,d7e47500-712e-11eb-828c-0aa1402f250c,d92d48b0-712e-11eb-82ce-0aa1402f250c,d7e53850-712e-11eb-8291-0aa1402f250c,2bbde440-1c4c-11ec-8bdc-200114580204,77812da0-05a5-11ec-a10a-200114580204,9427ae9a-573f-11ec-8cba-2ec189c3250c,c8e6843e-5455-11ec-a337-200114580204,d7ef4a70-712e-11eb-82bc-0aa1402f250c,f4d88285-16f0-11ed-a662-c0a80209250c,cefc6d5b-bc3e-11ec-9002-c0a80209250c,cf2e5986-bc3e-11ec-9002-c0a80209250c,229827bd-d7a2-11ec-ad50-c0a80209250c,c91ee9a8-5455-11ec-a337-200114580204,d816cfa0-712e-11eb-82ce-0aa1402f250c,56092540-05a7-11ec-a11d-200114580204,2b4dbd50-1c4c-11ec-8bdc-200114580204,d7e53850-712e-11eb-8290-0aa1402f250c,ce9825b6-b49c-11ec-adaf-c0a80209250c,d7e55f60-712e-11eb-8293-0aa1402f250c,d7ecd970-712e-11eb-82ad-0aa1402f250c,4e90bcf8-16f4-11ed-a674-c0a80209250c,d7eefc50-712e-11eb-82b9-0aa1402f250c,11130530-161b-11ec-aa64-200114580202,d7e55f60-712e-11eb-8294-0aa1402f250c,ea45e7f3-e656-11ec-87b9-c0a80209250c,d7e53850-712e-11eb-8292-0aa1402f250c,d7e670d0-712e-11eb-829a-0aa1402f250c,2ae70c40-1c4c-11ec-8bdc-200114580204,2ba4b6f0-1c4c-11ec-8bdc-200114580204,77710100-05a5-11ec-a10a-200114580204,2b43f950-1c4c-11ec-8bdc-200114580204,2ad38440-1c4c-11ec-8bdc-200114580204,35148c72-a386-11ec-a3ed-808d14b6250c,d7ef7180-712e-11eb-82be-0aa1402f250c,d7e75b30-712e-11eb-829c-0aa1402f250c,cf1535ed-bc3e-11ec-9002-c0a80209250c,2b0087b0-1c4c-11ec-8bdc-200114580204,86b79790-3599-11ec-917b-200114580202,560d9210-05a7-11ec-a11d-200114580204,2baec910-1c4c-11ec-8bdc-200114580204,99d2bb30-05a5-11ec-a10f-200114580204,a453af59-d216-11ec-893a-c0a80209250c,d7e4c320-712e-11eb-828e-0aa1402f250c,d7e55f60-712e-11eb-8292-0aa1402f250c,d8619440-712e-11eb-82ce-0aa1402f250c,b6742ce0-11e6-11ec-b9a7-85295a4f250c,d7eb04b0-712e-11eb-82a7-0aa1402f250c,d7e31570-712e-11eb-8286-0aa1402f250c,d7eefc50-712e-11eb-82ba-0aa1402f250c,d7e3ffd0-712e-11eb-828c-0aa1402f250c,d7e51140-712e-11eb-8290-0aa1402f250c,d7e2ee60-712e-11eb-8286-0aa1402f250c,d7eadda0-712e-11eb-82a5-0aa1402f250c,d7ed0080-712e-11eb-82b0-0aa1402f250c,2bc7f660-1c4c-11ec-8bdc-200114580204,8e01be8d-d5fc-11ec-9c83-c0a80209250c,ffe2e3c0-711a-11eb-996f-200114580202,cf3bfeb7-bc3e-11ec-9002-c0a80209250c,b3098ab0-1fc8-11ec-961c-200114580d00,2fa48e09-d5f1-11ec-9c3c-c0a80209250c,786b0d2a-a5da-11ec-844a-869e1c12250c,ced6ac8c-bc3e-11ec-9002-c0a80209250c,ce982885-bc3e-11ec-9002-c0a80209250c,d7e6e600-712e-11eb-829a-0aa1402f250c,dd03179a-d512-11ec-9119-c0a80209250c,7d55dcdb-bff1-11ec-a184-c0a80209250c,c652d7e6-f2de-11ec-afb8-c0a80209250c,e4ed5bfa-bb1f-11ec-89da-c0a80209250c,0082f450-711b-11eb-996f-200114580202,94476574-e5cc-11ec-84f5-c0a80209250c,d7f4a1a0-712e-11eb-82ce-0aa1402f250c,d7edeae0-712e-11eb-82b0-0aa1402f250c,2b14fa10-1c4c-11ec-8bdc-200114580204,ffe1f960-711a-11eb-996f-200114580202,d7ec6440-712e-11eb-82ac-0aa1402f250c,9cebde4b-9893-11ec-8d5e-0aa14015250c,d7eb04b0-712e-11eb-82a6-0aa1402f250c,88e3fc50-05a8-11ec-a130-200114580204,a42df330-a50f-11ec-82c1-0aa1229c250c,2af19390-1c4c-11ec-8bdc-200114580204,d89eeb60-712e-11eb-82ce-0aa1402f250c,d7ef4a70-712e-11eb-82bd-0aa1402f250c,d7f6c480-712e-11eb-82ce-0aa1402f250c,ee44eef9-d122-11ec-86da-c0a80209250c,e6d7d5a8-9893-11ec-8d5e-0aa14014250c,0080aa60-711b-11eb-996f-200114580202,d7ea8f80-712e-11eb-82a4-0aa1402f250c,46b56bbb-013f-11ed-b9e5-c0a80209250c,d7ffec40-712e-11eb-82ce-0aa1402f250c,2b8aed60-1c4c-11ec-8bdc-200114580204,0054df70-711b-11eb-996f-200114580202,d7ef7180-712e-11eb-82bf-0aa1402f250c,779b1e40-05a5-11ec-a10a-200114580204,d7ec3d30-712e-11eb-82aa-0aa1402f250c,d7ec1620-712e-11eb-82aa-0aa1402f250c,9cee6ba2-9893-11ec-8d5e-0aa14015250c,95369e78-e1d4-11ec-a2bf-c0a80209250c,d7e84590-712e-11eb-829d-0aa1402f250c,2b3923e0-1c4c-11ec-8bdc-200114580204,d7eb04b0-712e-11eb-82a9-0aa1402f250c,88e55be0-05a8-11ec-a130-200114580204,db93a64c-16f5-11ed-a67a-c0a80209250c,776d7e90-05a5-11ec-a10a-200114580204,d7f2cce0-712e-11eb-82ce-0aa1402f250c,2aebee40-1c4c-11ec-8bdc-200114580204,2bcc8a40-1c4c-11ec-8bdc-200114580204,7b674170-05a5-11ec-a10a-200114580204,3cb2d1cf-9893-11ec-8d5e-0aa14014250c,90a74255-d683-11ec-a016-c0a80209250c,2b57a860-1c4c-11ec-8bdc-200114580204,58d50680-166a-11ed-a5a4-c0a80209250c,8657aeb1-020e-11ed-beee-c0a80209250c,9082d9c0-05a5-11ec-a10e-200114580204,2bc33b70-1c4c-11ec-8bdc-200114580204,d7e5ad80-712e-11eb-8297-0aa1402f250c,0bfe525b-9fd8-11ec-9eca-200114580202,4a5ea21a-d21c-11ec-894c-c0a80209250c,99d63da0-05a5-11ec-a10f-200114580204,f2ae979c-16f0-11ed-a662-c0a80209250c,73fe7593-d683-11ec-a015-c0a80209250c,d7ecd970-712e-11eb-82af-0aa1402f250c,cf21e63c-bc3e-11ec-9002-c0a80209250c,d7e73420-712e-11eb-829b-0aa1402f250c,d7e1dcf0-712e-11eb-8283-0aa1402f250c,d7ee11f0-712e-11eb-82b0-0aa1402f250c,d7e7d060-712e-11eb-829c-0aa1402f250c,ca4479a4-5455-11ec-a337-200114580204,560b9640-05a7-11ec-a11d-200114580204,ff57bac0-711a-11eb-996f-200114580202,d7ec6440-712e-11eb-82ab-0aa1402f250c,ff5793b0-711a-11eb-996f-200114580202,ffdf6150-711a-11eb-996f-200114580202,e6da2748-9893-11ec-8d5e-0aa14014250c,2ba94ad0-1c4c-11ec-8bdc-200114580204,d7ef2360-712e-11eb-82bc-0aa1402f250c,1632e031-f386-11ec-b836-c0a80209250c,3caee8da-9893-11ec-8d5e-0aa14014250c,2b48db50-1c4c-11ec-8bdc-200114580204,9084ae80-05a5-11ec-a10f-200114580204,b2987f3a-d36d-11ec-8bf2-c0a80209250c,7b693d40-05a5-11ec-a10a-200114580204,007fc000-711b-11eb-996f-200114580202,2b5cb170-1c4c-11ec-8bdc-200114580204,cfbca486-1486-11ed-a0f2-c0a80209250c,d7eadda0-712e-11eb-82a4-0aa1402f250c,2abd8b40-1c4c-11ec-8bdc-200114580204,a63e986e-1661-11ed-a464-c0a80209250c,2ad8b460-1c4c-11ec-8bdc-200114580204,d7ec3d30-712e-11eb-82ab-0aa1402f250c,2ae25150-1c4c-11ec-8bdc-200114580204,88e6e280-05a8-11ec-a130-200114580204,6c30f304-fbc9-11ec-9050-c0a80209250c,ceb12202-bc3e-11ec-9002-c0a80209250c,d9af49f0-712e-11eb-82ce-0aa1402f250c,d7e36390-712e-11eb-8287-0aa1402f250c,d7e81e80-712e-11eb-829c-0aa1402f250c,f14a416d-cc71-11ec-988e-c0a80209250c,d7e73420-712e-11eb-829a-0aa1402f250c,d7e8bac0-712e-11eb-829e-0aa1402f250c,9ce9f624-9893-11ec-8d5e-0aa14015250c,d9587760-712e-11eb-82ce-0aa1402f250c,45794039-d4af-11ec-8d4a-c0a80209250c,d7e58670-712e-11eb-8294-0aa1402f250c,947f40a6-e5cc-11ec-84f5-c0a80209250c,28a9283f-5e4b-11ec-9864-808d13fc250c,d7e49c10-712e-11eb-828e-0aa1402f250c,9082d9c0-05a5-11ec-a10d-200114580204,d7e86ca0-712e-11eb-829e-0aa1402f250c,4f2ddf20-712f-11eb-82cf-0aa1402f250c,d7e73420-712e-11eb-829c-0aa1402f250c,d7ebef10-712e-11eb-82a9-0aa1402f250c,776b82c0-05a5-11ec-a10a-200114580204,2b288210-1c4c-11ec-8bdc-200114580204,2b719900-1c4c-11ec-8bdc-200114580204,d7e9f340-712e-11eb-82a0-0aa1402f250c,bdbc6ca8-f89e-11ec-8bc9-c0a80209250c,d7eb04b0-712e-11eb-82a8-0aa1402f250c,d7e9cc30-712e-11eb-829f-0aa1402f250c,f4cca637-ccbf-11ec-98da-c0a80209250c,776b82c0-05a5-11ec-a109-200114580204,2b32bb40-1c4c-11ec-8bdc-200114580204,d7f0d110-712e-11eb-82c8-0aa1402f250c,c98c6366-5455-11ec-a337-200114580204,2b7b8410-1c4c-11ec-8bdc-200114580204,3cb01e57-9893-11ec-8d5e-0aa14014250c,c9fba6b5-5455-11ec-a337-200114580204,3cb1036d-9893-11ec-8d5e-0aa14014250c,d5bb25b0-9246-11eb-80d6-2a01cb0404fc,d7e2ee60-712e-11eb-8285-0aa1402f250c,47b86903-dac9-11ec-becf-c0a80209250c,2bb3ab10-1c4c-11ec-8bdc-200114580204,ce668283-bc3e-11ec-9002-c0a80209250c,ffde9e00-711a-11eb-996f-200114580202,ff582ff0-711a-11eb-996f-200114580202,d806f120-712e-11eb-82ce-0aa1402f250c,7decf08e-d817-11ec-b68a-c0a80209250c,d7ec8b50-712e-11eb-82ac-0aa1402f250c,d46aa7d3-1486-11ed-a0f2-c0a80209250c,4e0c0440-8ce2-11eb-bafe-2a010e0a09fb,d7eed540-712e-11eb-82b2-0aa1402f250c,d7ed75b0-712e-11eb-82b0-0aa1402f250c,d7e5fba0-712e-11eb-829a-0aa1402f250c,6f395e9d-013e-11ed-b9e1-c0a80209250c,bb208fcb-d0a2-11ec-80f6-c0a80209250c,ea805f16-e656-11ec-87b9-c0a80209250c,8b5379d8-55c2-11ec-8acf-2a010e0a0b16,d1d345c0-d4ae-11ec-8d4a-c0a80209250c,d7ea6870-712e-11eb-82a4-0aa1402f250c,00847af0-711b-11eb-996f-200114580202,c85ec27c-f384-11ec-b836-c0a80209250c,d7ee11f0-712e-11eb-82b1-0aa1402f250c,d7e5d490-712e-11eb-829a-0aa1402f250c,d821a510-712e-11eb-82ce-0aa1402f250c,6409b34f-f2df-11ec-afd3-c0a80209250c,d7eefc50-712e-11eb-82b7-0aa1402f250c,c846dd4d-5455-11ec-a337-200114580204,52781b60-144e-11ed-a0e9-c0a80209250c,6d53e1f8-13c9-11ed-8a4a-c0a80209250c,03534820-21f0-11ec-a23d-808d13fc250c,dd00e865-d06e-11ec-b606-c0a80209250c,00359646-a50c-11ec-82bf-0aa1229c250c,cee35485-bc3e-11ec-9002-c0a80209250c,e6de354c-9893-11ec-8d5e-0aa14014250c,cb5e4b29-1486-11ed-a0f2-c0a80209250c,2b1a2a30-1c4c-11ec-8bdc-200114580204,776da5a0-05a5-11ec-a10a-200114580204,009061d0-711b-11eb-996f-200114580202,496cce77-dfa6-11ec-923a-c0a80209250c,c8126549-8f1e-11ec-9ff2-c08714f4250c,d7e86ca0-712e-11eb-829d-0aa1402f250c,2b85e450-1c4c-11ec-8bdc-200114580204,ffe024a0-711a-11eb-996f-200114580202,2b767b00-1c4c-11ec-8bdc-200114580204,d7e78240-712e-11eb-829c-0aa1402f250c,d7fae330-712e-11eb-82ce-0aa1402f250c,3f43ec72-a458-11ec-81db-808d14b6250c,7289809c-a6b5-11ec-868a-808d14b6250c,9084ae80-05a5-11ec-a10e-200114580204,d8b113d0-712e-11eb-82ce-0aa1402f250c,fe946ac0-711a-11eb-996f-200114580202,4adb8576-d21c-11ec-894c-c0a80209250c,d7e5ad80-712e-11eb-8298-0aa1402f250c,75770a62-c23a-11ec-884c-c0a80209250c,d36b477d-a392-11ec-a41e-869e1ccf250c,d7e58670-712e-11eb-8295-0aa1402f250c,2ab8a940-1c4c-11ec-8bdc-200114580204,950b6d2f-e5cc-11ec-84f5-c0a80209250c,d7f257b0-712e-11eb-82cb-0aa1402f250c,d7e36390-712e-11eb-8286-0aa1402f250c,084594f0-2a79-11ec-9e6f-85295a4f250c,dfe0f150-0a82-11ec-a525-b9cf5815250c,6bef1756-cfa7-11ec-a067-c0a80209250c,d7e4ea30-712e-11eb-8290-0aa1402f250c,d7edc3d0-712e-11eb-82b0-0aa1402f250c,3cb1cd7a-9893-11ec-8d5e-0aa14014250c,d7dc1090-712e-11eb-8283-0aa1402f250c,d7ea6870-712e-11eb-82a1-0aa1402f250c,d7efe6b0-712e-11eb-82c8-0aa1402f250c,2b3ec930-1c4c-11ec-8bdc-200114580204,9e832036-f6e0-11ec-89aa-c0a80209250c,d88e2280-712e-11eb-82ce-0aa1402f250c,94b9c92c-e5cc-11ec-84f5-c0a80209250c,d7efe6b0-712e-11eb-82c7-0aa1402f250c,2e5c918e-013f-11ed-b9e3-c0a80209250c,2add6f50-1c4c-11ec-8bdc-200114580204,d7efbfa0-712e-11eb-82c6-0aa1402f250c,d7e47500-712e-11eb-828d-0aa1402f250c,d7e9f340-712e-11eb-829f-0aa1402f250c,34011532-9b4e-11ec-9479-2a0209080183,ce172f10-9b09-11ec-9386-2a0209080183,00858c60-711b-11eb-996f-200114580202,62cbf860-df7e-11eb-88b5-200102f801c1,d7eefc50-712e-11eb-82b8-0aa1402f250c,d2157d58-1486-11ed-a0f2-c0a80209250c,0afcffb2-e1d2-11ec-a2be-c0a80209250c,e6dee42d-9893-11ec-8d5e-0aa14014250c,d7ef9890-712e-11eb-82c5-0aa1402f250c,d7efbfa0-712e-11eb-82c7-0aa1402f250c,d7e44df0-712e-11eb-828c-0aa1402f250c,d7e84590-712e-11eb-829c-0aa1402f250c,70082e40-120a-11ec-b9ab-4f590d6c250c,d7f9f8d0-712e-11eb-82ce-0aa1402f250c,2b919c70-4251-11ec-b6d2-5f54dcbf250c,2cb9e02b-a14b-11ec-a076-511cc1ec250c,ceefe3bf-bc3e-11ec-9002-c0a80209250c,7a49b00f-bc3f-11ec-9004-c0a80209250c,4025aae2-a458-11ec-81db-808d14b6250c,c872a104-5455-11ec-a337-200114580204,c3ab8fe2-bef6-11ec-9fbd-c0a80209250c,d7e2c750-712e-11eb-8283-0aa1402f250c,f2c632c5-16f0-11ed-a662-c0a80209250c,d7ea4160-712e-11eb-82a0-0aa1402f250c,d7f47a90-712e-11eb-82ce-0aa1402f250c,1c0cd4d0-1615-11ec-a8d3-808d13fc250c,bf85b066-1486-11ed-a0f2-c0a80209250c,d6ae5ff5-1486-11ed-a0f2-c0a80209250c,d7e5d490-712e-11eb-8298-0aa1402f250c,d84f92e0-712e-11eb-82ce-0aa1402f250c,0abc5e34-a926-11ec-9048-200300ce7f10,f63194e2-07d7-11ed-8753-c0a80209250c,2ac8fcf0-1c4c-11ec-8bdc-200114580204,d7f0f820-712e-11eb-82c8-0aa1402f250c,8a144d1f-bc3f-11ec-9005-c0a80209250c,a64ea788-af3a-11ec-9209-808d13fc250c,2b810250-1c4c-11ec-8bdc-200114580204,d7ee11f0-712e-11eb-82b2-0aa1402f250c,d7f0aa00-712e-11eb-82c8-0aa1402f250c,002543f0-711b-11eb-996f-200114580202,cea4aa9c-bc3e-11ec-9002-c0a80209250c,2a626fe0-1fa2-11ec-956b-200114580202,7e5273cd-f2df-11ec-afe8-c0a80209250c,ff571e80-711a-11eb-996f-200114580202,428fed80-1238-11ec-ba5a-808d14b6250c,0049bbe0-711b-11eb-996f-200114580202,429062b0-1238-11ec-ba5a-808d14b6250c,fe5a6227-a468-11ec-8207-8d02f3c0250c,ce731f63-bc3e-11ec-9002-c0a80209250c,c94a9df1-1486-11ed-a0f2-c0a80209250c,2b6c8ff0-1c4c-11ec-8bdc-200114580204,b43ba300-e737-11ec-8ae9-c0a80209250c,a0caca82-b9ae-11ec-863f-c0a80209250c,48b173a0-2c69-11ec-8b38-2a010e0a09fb,ffb3e480-711a-11eb-996f-200114580202,ce7f62e7-bc3e-11ec-9002-c0a80209250c,6789f887-93e8-11ec-b3e8-869e1c12250c,4a62e128-d21c-11ec-894c-c0a80209250c,d7f64f50-712e-11eb-82ce-0aa1402f250c,d7ef9890-712e-11eb-82c4-0aa1402f250c,1f71c895-c578-11ec-bf73-c0a80209250c,cf08be5f-bc3e-11ec-9002-c0a80209250c,9461fac5-e5cc-11ec-84f5-c0a80209250c,4a601e75-d21c-11ec-894c-c0a80209250c,4a61a97c-d21c-11ec-894c-c0a80209250c,c79048f2-8f1e-11ec-9ff2-c08714f4250c,00823100-711b-11eb-996f-200114580202,c943f4de-8cfb-11ec-9c69-25fc50f9250c,0a3c5246-51f5-11ec-9f6a-808d13fc250c,da3372e7-d4ae-11ec-8d4a-c0a80209250c,d7f230a0-712e-11eb-82ca-0aa1402f250c,c1d125c6-d92a-11ec-b9ac-c0a80209250c,2b52c660-1c4c-11ec-8bdc-200114580204,d7e2c750-712e-11eb-8284-0aa1402f250c,d7ecd970-712e-11eb-82b0-0aa1402f250c,ee7c66a6-a14b-11ec-a078-511cc1ec250c,32e8c987-16f4-11ed-a674-c0a80209250c,c2c4b160-1486-11ed-a0f2-c0a80209250c,3cec691f-b71b-11ec-b89d-c0a80209250c,d7ea6870-712e-11eb-82a2-0aa1402f250c,d7e9cc30-712e-11eb-829e-0aa1402f250c,c7aa7049-8f1e-11ec-9ff2-c08714f4250c,f4ce5789-16f0-11ed-a662-c0a80209250c,21710de8-fe39-11ec-a120-c0a80209250c,008146a0-711b-11eb-996f-200114580202,8c2799ba-15d0-11ed-a356-c0a80209250c,d7f257b0-712e-11eb-82cc-0aa1402f250c,d7eadda0-712e-11eb-82a6-0aa1402f250c,d911f880-712e-11eb-82ce-0aa1402f250c,d7f230a0-712e-11eb-82cb-0aa1402f250c,3e791883-a386-11ec-a3ed-808d14b6250c,227b97ce-d043-11ec-b500-c0a80209250c,1cf5d240-1b18-11ec-85cf-2a010e0a09fb,f510ed70-a396-11ec-a42e-808d14b6250c,d7e33c80-712e-11eb-8286-0aa1402f250c,d7f2cce0-712e-11eb-82cd-0aa1402f250c,ffb459b0-711a-11eb-996f-200114580202,f180a91e-cc71-11ec-988e-c0a80209250c,d7eefc50-712e-11eb-82b6-0aa1402f250c,d7f0f820-712e-11eb-82c9-0aa1402f250c,c71e4dde-1486-11ed-a0f2-c0a80209250c,446a2cb0-17d3-11ec-9f26-2a010e0a09fb,d7dc1090-712e-11eb-8281-0aa1402f250c,50b9b56b-0ddd-11ed-86c0-c0a80209250c,1ca1dbb1-b71d-11ec-b8a5-c0a80209250c,2ace2d10-1c4c-11ec-8bdc-200114580204,f14ad2e3-cc71-11ec-988e-c0a80209250c,d7e3ffd0-712e-11eb-828a-0aa1402f250c,4f2ccdb0-712f-11eb-82cf-0aa1402f250c,d7eed540-712e-11eb-82b3-0aa1402f250c,2b622fb0-1c4c-11ec-8bdc-200114580204,c7de03aa-8f1e-11ec-9ff2-c08714f4250c,d6b92fd0-2df9-11ec-8fef-511cc1ec250c,c091d5f0-e05f-11eb-8b28-200102f801c1,d7e38aa0-712e-11eb-8288-0aa1402f250c,d7f95c90-712e-11eb-82ce-0aa1402f250c,d86f9e00-712e-11eb-82ce-0aa1402f250c,27687044-c575-11ec-bf64-c0a80209250c,d7e38aa0-712e-11eb-8289-0aa1402f250c,906b6d3a-b18d-11ec-9cd1-c0a80209250c,d7f712a0-712e-11eb-82ce-0aa1402f250c,9b70ca90-2e04-11ec-8ffb-511cc1ec250c,d7f2a5d0-712e-11eb-82cd-0aa1402f250c,d7f0f820-712e-11eb-82ca-0aa1402f250c,c7c49015-8f1e-11ec-9ff2-c08714f4250c,28a4ef61-5e4b-11ec-9864-808d13fc250c,c7f781c6-8f1e-11ec-9ff2-c08714f4250c,3f9e2220-1c67-11ec-8ca3-200114580202,d9f2bb90-712e-11eb-82ce-0aa1402f250c,c4e593fa-1486-11ed-a0f2-c0a80209250c,d7e893b0-712e-11eb-829e-0aa1402f250c,d7eed540-712e-11eb-82b4-0aa1402f250c,d7ea6870-712e-11eb-82a3-0aa1402f250c,d828d100-712e-11eb-82ce-0aa1402f250c,866dfffe-16f0-11ed-a65e-c0a80209250c,d7ea6870-712e-11eb-82a0-0aa1402f250c,f4c21d30-16f0-11ed-a662-c0a80209250c,7f5b17d0-712f-11eb-82d3-0aa1402f250c,3f80c900-dae5-11ec-bf0f-c0a80209250c,8bc75650-310f-11ec-a4a0-c20cb928250c,d7ef7180-712e-11eb-82c1-0aa1402f250c,d7efbfa0-712e-11eb-82c5-0aa1402f250c,7d5d9f2e-d5bd-11ec-977d-c0a80209250c";
// std::string ccdbPathsCS = R"(http://ccdb-test.cern.ch:8080/TPC/Calib/CorrectionMaps/1613573276947/4fe98e00-712f-11eb-82cf-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Sigma_1/1646051868701/3cb3dfd8-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5n-fs1e30-ht200-qs0e23s23e22-nb233-pidlhc11dv3en-pt100_ptrg.r5772/1/2b0b0f00-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Sigma_2/1646051868701/3cb4e5a7-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5n-fs1e30-ht200-qs0e29s29e28-nb233-pidlhc11dv4en-pt100_ptrg.r5773/1/2b9a7dc0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/Calib/NoiseMapMCM/1/4b62d089-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/PIDThresholds/1613573156848/d7e3ffd0-712e-11eb-828b-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/MO/BatchTestTaskRsqcE/example/1629896934962/99d48ff0-05a5-11ec-a10f-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb22_trkl-b5p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1-pt100_ptrg.r5037/1/2b675fd0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/ITS/Calib/DeadMap/1613573157226/d9cfa330-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Sigma_4/1646051868701/3cb5b0e4-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/PHS/Calib/Align/1/ce8bc31d-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Sigma_5/1646052030150/9ced7ab8-9893-11ec-8d5e-0aa14015250c,http://ccdb-test.cern.ch:8080/FT0/Calib/PMTrends/1613564635187/0087d650-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/HMP/Calib/Masked/1613573156886/d7eb2bc0-712e-11eb-82a9-0aa1402f250c,http://ccdb-test.cern.ch:8080/ITS/Calib/ITHR/1654849604888/106111a1-e897-11ec-8f1d-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Calib/Nmean/1613573156887/d7ec1620-712e-11eb-82a9-0aa1402f250c,http://ccdb-test.cern.ch:8080/ITS/Calib/NoiseMap/1613573157318/da68fd00-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/ITS/Calib/NULL/1657283995013/11fac573-febb-11ec-a565-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5p-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5549/1/2b2dd940-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/FT0/Calib/TimeDelays/1613573156912/d7ef4a70-712e-11eb-82be-0aa1402f250c,http://ccdb-test.cern.ch:8080/MCH/Calib/HV/1647424719983/a86d5475-a50f-11ec-82c1-0aa1229c250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Mean_5/1646052030150/9ceaf470-9893-11ec-8d5e-0aa14015250c,http://ccdb-test.cern.ch:8080/MCH/BadChannelCalib/1646911677508/48e12533-a065-11ec-9f44-808d13fc250c,http://ccdb-test.cern.ch:8080/ITS/Calib/Params/1613573156811/d7dc1090-712e-11eb-8282-0aa1402f250c,http://ccdb-test.cern.ch:8080/MCH/Calib/RejectList/1613573156893/d7ecd970-712e-11eb-82ae-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b2p-fs1e30-ht200-qs0e29s29e28-nb233-pidlhc11dv4en-pt100_ptrg.r5773/1/2afbccc0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/MCH/Config/DCSDPconfig/1647264611784/e019d73f-a39a-11ec-800e-869e1ccf250c,http://ccdb-test.cern.ch:8080/FV0/Calib/Thresholds/1613573156913/d7ef9890-712e-11eb-82c2-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofiePressure/1613573156853/d7e58670-712e-11eb-8296-0aa1402f250c,http://ccdb-test.cern.ch:8080/FV0/Calib/TimeDelays/1613573156913/d7ef9890-712e-11eb-82c3-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_gaschromatographXe/1613573156851/d7e4c320-712e-11eb-828f-0aa1402f250c,http://ccdb-test.cern.ch:8080/GLO/Calib/GRPMagField/1647345946177/3f444131-a458-11ec-81db-808d14b6250c,http://ccdb-test.cern.ch:8080/FV0/Calib/TimeSlewing/1613573156973/d80bac10-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/PadNoise/1/4b5d3497-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb26_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5771/1/2b954da0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb26_trkl-b5p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5771/1/2b101810-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5767/1/2bb88d10-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2n-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5585/1/2b1e21d0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/FV0/Calib/PMGains/1613573156912/d7ef7180-712e-11eb-82c0-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1-pt100_ptrg.r5762/1/2b05b7d0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/EMC/FEEDCS/61592227200/5cb545da-dabf-11ec-bea2-c0a80209250c,http://ccdb-test.cern.ch:8080/FV0/Calib/PMTrends/1613564635214/008bf500-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TPC/Calib/CorrectionMapsRef/1613573157200/d98839f0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/PIDLQ/1613573156944/d7f8e760-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/FT0/LookUpTableNew/1633004500028/f6eb7c80-21e8-11ec-a1bc-808d13fc250c,http://ccdb-test.cern.ch:8080/TRD/Calib/PIDLQ1D/1613573156861/d7e6bef0-712e-11eb-829a-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Mean_4/1646052154191/e6daf742-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/FT0/Calib/RecoParam/1613573156910/d7eefc50-712e-11eb-82bb-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/DCSconfig/1648812366495/657decc0-b1ae-11ec-9d4d-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Config/DCSDPconfig/1640991600000/e25956d1-c169-11ec-82c1-c0a80209250c,http://ccdb-test.cern.ch:8080/ITS/Config/ClustererParam/1/4e2ab4e2-d4af-11ec-8d4a-c0a80209250c,http://ccdb-test.cern.ch:8080/ITS/DCS_CONFIG/1659617970139/473d5994-13f5-11ed-a09e-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Config/TrapConfig/1/6d4bfe90-1c49-11ec-8bd2-200114580204,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Gaintbl_Uniform_FGAN0_2011-01/1/ca25540b-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/ZDC/Calib/EnergyCalib/1550600800000/cd7761b1-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/FT0/Calib/Saturation/1613573156910/d7ef2360-712e-11eb-82bb-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Sigma_0/1646052154191/e6dbfbde-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Sigma_1/1646052154191/e6dd24b4-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Mean_1/1646052154191/e6d93534-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/TRD/Calib/PIDNN/1613573156916/d7f2a5d0-712e-11eb-82cc-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Krypton_2011-03/1/c89d711c-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/FV0/Calib/PulseShapes/1613573156913/d7ef7180-712e-11eb-82c2-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b0-fs1e24-ht200-qs0e24s24e23-pidlinear_ptrg.r5570/1/2b9fd4f0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/QO/CheckSeparately/skeletonTask/example/1629896877135/777f31d0-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b00-fs1e30-ht200-qs0e23s23e22-nb233-pidlhc11dv3en_ptrg.r5772/1/2af6eac0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/Calib/PadStatus/1/4b6190e8-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Temperature/1652109651005/6bf191a8-cfa7-11ec-a067-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_gasCO2/1613573156851/d7e4ea30-712e-11eb-828f-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofieTemp/1613573156853/d7e58670-712e-11eb-8297-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5n-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5505/1/2ab41560-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/MCH/Calib/OccupancyMap/1613573156920/d7f564f0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/test_Calib/NoiseMap/1649940704543/a5b974de-bbf1-11ec-8e56-c0a80209250c,http://ccdb-test.cern.ch:8080/MCH/Calib/Align/1/ceca30b9-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/GLO/Calib/LHCIF/1647345946177/3f44b099-a458-11ec-81db-808d14b6250c,http://ccdb-test.cern.ch:8080/TRD/Calib/PRFWidth/1613573156855/d7e5d490-712e-11eb-8299-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/DetNoise/1613573156843/d7e38aa0-712e-11eb-828a-0aa1402f250c,http://ccdb-test.cern.ch:8080/ITS/Calib/THR/1659891851005/f5ba315d-1672-11ed-a5ad-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/NoiseStatusMCM/1/079f32e3-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5p-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5505/1/2b232ae0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Sigma_3/1646052030150/9cec9482-9893-11ec-8d5e-0aa14015250c,http://ccdb-test.cern.ch:8080/ITS/Calib/Threshold/1639762935401/ac5fb7f8-5f60-11ec-9a09-808d5c3b250c,http://ccdb-test.cern.ch:8080/TRD/Calib/HalfChamberStatusQC/1577833200000/e5defba8-eebc-11ec-8d02-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/RecoParam/1613573156849/d7e47500-712e-11eb-828c-0aa1402f250c,http://ccdb-test.cern.ch:8080/MCH/Calib/Pedestals/1613573157153/d92d48b0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_gasH2O/1613573156851/d7e53850-712e-11eb-8291-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb26_trkl-b2n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5771/1/2bbde440-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/QO/CheckSeparately/skeletonTask/example2/1629896877376/77812da0-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/TPC/Calib/dEdx/6/9427ae9a-573f-11ec-8cba-2ec189c3250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Krypton_2012-01/1/c8e6843e-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/FT0/Calib/Thresholds/1613573156910/d7ef4a70-712e-11eb-82bc-0aa1402f250c,http://ccdb-test.cern.ch:8080/GLO/Calib/MeanVertex/1659943438117/f4d88285-16f0-11ed-a662-c0a80209250c,http://ccdb-test.cern.ch:8080/FV0/Calib/Align/1/cefc6d5b-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/TRK/Calib/Align/1/cf2e5986-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/FV0/Calib/ChannelTimeOffset/1546300800/229827bd-d7a2-11ec-ad50-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Krypton_2015-01/1/c91ee9a8-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/TPC/Calib/GainFactorDedx/1613573156990/d816cfa0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/QO/FunctionalTestl1XcV/1629897680002/56092540-05a7-11ec-a11d-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb26_trkl-b2p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5771/1/2b4dbd50-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_gasO2/1613573156852/d7e53850-712e-11eb-8290-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/Config/NoiseMap/1649134606831/ce9825b6-b49c-11ec-adaf-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofieGain/1613573156852/d7e55f60-712e-11eb-8293-0aa1402f250c,http://ccdb-test.cern.ch:8080/MCH/Calib/RecoParam/1613573156892/d7ecd970-712e-11eb-82ad-0aa1402f250c,http://ccdb-test.cern.ch:8080/CTP/Config/Config/1659946806421/4e90bcf8-16f4-11ed-a674-c0a80209250c,http://ccdb-test.cern.ch:8080/FT0/Calib/LightYields/1613573156909/d7eefc50-712e-11eb-82b9-0aa1402f250c,http://ccdb-test.cern.ch:8080/CTP/OrbitReset/0/11130530-161b-11ec-aa64-200114580202,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofieHv/1613573156852/d7e55f60-712e-11eb-8294-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/Config/Params/1654602145186/ea45e7f3-e656-11ec-87b9-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_gasOverpressure/1613573156852/d7e53850-712e-11eb-8292-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_envTemp/1613573156859/d7e670d0-712e-11eb-829a-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_p_nozs_tb30_trk_ptrg.r4850/1/2ae70c40-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b2n-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5566/1/2ba4b6f0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/QO/ABCCheck/1629896877270/77710100-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b2n-fs1e30-ht200-qs0e23s23e22-nb233-pidlhc11dv3en-pt100_ptrg.r5772/1/2b43f950-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b2p-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5566/1/2ad38440-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/GLO/Config/EnvVars/1647255734792/35148c72-a386-11ec-a3ed-808d14b6250c,http://ccdb-test.cern.ch:8080/FV0/Calib/ChargeEqualization/1613573156912/d7ef7180-712e-11eb-82be-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Calib/Status/1613573156864/d7e75b30-712e-11eb-829c-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/Calib/Align/1/cf1535ed-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b2p-fs1e30-ht200-qs0e23s23e22-nb233-pidlhc11dv3en-pt100_ptrg.r5772/1/2b0087b0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/GLO/Config/Geometry/1635169403018/86b79790-3599-11ec-917b-200114580202,http://ccdb-test.cern.ch:8080/TST/MO/FunctionalTestl1XcV/example2/1629897680241/560d9210-05a7-11ec-a11d-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b2n-fs1e30-ht200-qs0e29s29e28-nb233-pidlhc11dv4en-pt100_ptrg.r5773/1/2baec910-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/QO/BatchTestCheckRsqcE/1629896934729/99d2bb30-05a5-11ec-a10f-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/Foo/1/a453af59-d216-11ec-893a-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_gaschromatographCO2/1613573156851/d7e4c320-712e-11eb-828e-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofieCO2/1613573156852/d7e55f60-712e-11eb-8292-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/TrapConfig/1613573157049/d8619440-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/DCSconfig/1631244313972/b6742ce0-11e6-11ec-b9a7-85295a4f250c,http://ccdb-test.cern.ch:8080/CPV/Calib/BadChannels/1613573156884/d7eb04b0-712e-11eb-82a7-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/Align/Data/1613573156825/d7e31570-712e-11eb-8286-0aa1402f250c,http://ccdb-test.cern.ch:8080/FT0/Calib/PMGains/1613573156909/d7eefc50-712e-11eb-82ba-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/Temperature/1613573156845/d7e3ffd0-712e-11eb-828c-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_gaschromatographN2/1613573156851/d7e51140-712e-11eb-8290-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/TimeDrift/1613573156841/d7e2ee60-712e-11eb-8286-0aa1402f250c,http://ccdb-test.cern.ch:8080/CPV/Align/Data/1613573156882/d7eadda0-712e-11eb-82a5-0aa1402f250c,http://ccdb-test.cern.ch:8080/MID/Align/Data/1613573156896/d7ed0080-712e-11eb-82b0-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b0-fs1e24-ht200-qs0e24s24e23-pidlinear_ptrg.r5505/1/2bc7f660-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/CPV/Config/CPVCalibParams/946684800000/8e01be8d-d5fc-11ec-9c83-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/T0Fill/1613564634102/ffe2e3c0-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/FT3/Calib/Align/1/cf3bfeb7-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/TST/test/1632770738347/b3098ab0-1fc8-11ec-961c-200114580d00,http://ccdb-test.cern.ch:8080/CPV/Config/CPVSimParams/946684800000/2fa48e09-d5f1-11ec-9c3c-c0a80209250c,http://ccdb-test.cern.ch:8080/MID/BadChannels/8/786b0d2a-a5da-11ec-844a-869e1c12250c,http://ccdb-test.cern.ch:8080/MID/Calib/Align/1/ced6ac8c-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/CPV/Calib/Align/1/ce982885-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Align/Data/1613573156863/d7e6e600-712e-11eb-829a-0aa1402f250c,http://ccdb-test.cern.ch:8080/MID/Calib/BadChannels/1/dd03179a-d512-11ec-9119-c0a80209250c,http://ccdb-test.cern.ch:8080/MFT/test_Config/NoiseMap/1650380442527/7d55dcdb-bff1-11ec-a184-c0a80209250c,http://ccdb-test.cern.ch:8080/CPV/Calib/BadChannelMap/1655979916066/c652d7e6-f2de-11ec-afb8-c0a80209250c,http://ccdb-test.cern.ch:8080/FV0/Align/1640991600000/e4ed5bfa-bb1f-11ec-89da-c0a80209250c,http://ccdb-test.cern.ch:8080/ZDC/Align/Data/1613564635156/0082f450-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/CPV/PedestalRun/ChannelEfficiencies/1654542736364/94476574-e5cc-11ec-84f5-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Calib/NoiseMap/1613573156919/d7f4a1a0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/MCH/Calib/LocalTriggerBoardMasks/1613573156901/d7edeae0-712e-11eb-82b0-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5p-fs1e30-ht200-qs0e23s23e22-nb233-pidlhc11dv3en-pt100_ptrg.r5772/1/2b14fa10-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TOF/Calib/T0FillOnlineCalib/1613564634100/ffe1f960-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/HMP/Calib/QeMap/1613573156889/d7ec6440-712e-11eb-82ac-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Mean_6/1646052030150/9cebde4b-9893-11ec-8d5e-0aa14015250c,http://ccdb-test.cern.ch:8080/CPV/Calib/RecoParam/1613573156884/d7eb04b0-712e-11eb-82a6-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/QO/QcCheck/1629898194804/88e3fc50-05a8-11ec-a130-200114580204,http://ccdb-test.cern.ch:8080/MCH/Calib/LV/1647424713906/a42df330-a50f-11ec-82c1-0aa1229c250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b0-fs1e24-ht200-qs0e24s24e23-pidlinear_ptrg.r5549/1/2af19390-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/FT0/Calib/TimeSlewing/1613573157078/d89eeb60-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/FV0/Align/Data/1613573156912/d7ef4a70-712e-11eb-82bd-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/PidResponse/1613573156922/d7f6c480-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/MID/Calib/ElectronicsMasks/1652262457977/ee44eef9-d122-11ec-86da-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Mean_0/1646052154191/e6d7d5a8-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/MID/Calib/GlobalTriggerBoardMasks/1613564635140/0080aa60-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/PHS/Calib/BadChannels/1613573156880/d7ea8f80-712e-11eb-82a4-0aa1402f250c,http://ccdb-test.cern.ch:8080/FT0/Config/DCSDPconfig/1657560679445/46b56bbb-013f-11ed-b9e5-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/ParOffline/1613573156959/d7ffec40-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b0-fs1e30-ht200-qs0e29s29e28-nb233-pidlhc11dv4en_ptrg.r5773/1/2b8aed60-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/MCH/Calib/MappingData/1613564634832/0054df70-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/FV0/Calib/LightYields/1613573156912/d7ef7180-712e-11eb-82bf-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/QO/XYZCheck/1629896877544/779b1e40-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/CPV/Calib/Time/1613573156887/d7ec3d30-712e-11eb-82aa-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Calib/Qthre/1613573156887/d7ec1620-712e-11eb-82aa-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Sigma_6/1646052030150/9cee6ba2-9893-11ec-8d5e-0aa14015250c,http://ccdb-test.cern.ch:8080/FT0/Config/LookupTable/1654106369429/95369e78-e1d4-11ec-a2bf-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/RecoParam/1613573156862/d7e84590-712e-11eb-829d-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5p-fs1e30-ht200-qs0e29s29e28-nb233-pidlhc11dv4en-pt100_ptrg.r5773/1/2b3923e0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/HMP/Calib/RecoParam/1613573156885/d7eb04b0-712e-11eb-82a9-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/MO/QcTask/example/1629898195040/88e55be0-05a8-11ec-a130-200114580204,http://ccdb-test.cern.ch:8080/CTP/Calib/Scalers/1659946806421/db93a64c-16f5-11ed-a67a-c0a80209250c,http://ccdb-test.cern.ch:8080/TST/MO/QYZTask/example2/1629896877246/776d7e90-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/MCH/Calib/MappingRunData/1613573156918/d7f2cce0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv2en-pt100_ptrg.r5764/1/2aebee40-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b0-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1_ptrg.r5762/1/2bcc8a40-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/MO/skeletonTask/example/1629896883915/7b674170-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Sigma_0/1646051868701/3cb2d1cf-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/PHS/Calib/BadMap/1/90a74255-d683-11ec-a016-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1-pt100_ptrg.r4946/1/2b57a860-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/CPV/PhysicsRun/GainCalibData/1659888152827/58d50680-166a-11ed-a5a4-c0a80209250c,http://ccdb-test.cern.ch:8080/ITS/Calib/DIG/1657649690770/8657aeb1-020e-11ed-beee-c0a80209250c,http://ccdb-test.cern.ch:8080/TST/MO/MultiNodeRemoteTest30461/example/1629896919318/9082d9c0-05a5-11ec-a10e-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1-pt100_ptrg.r5762/1/2bc33b70-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofieVelocity/1613573156854/d7e5ad80-712e-11eb-8297-0aa1402f250c,http://ccdb-test.cern.ch:8080/ITS/Calib/ClusterDictionary/0/0bfe525b-9fd8-11ec-9eca-200114580202,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb30_trkl-b5n-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5549/1/4a5ea21a-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/TST/MO/BatchTestTaskRsqcE/example2/1629896934973/99d63da0-05a5-11ec-a10f-200114580204,http://ccdb-test.cern.ch:8080/PHS/BadMap/Occ/1659942822772/f2ae979c-16f0-11ed-a662-c0a80209250c,http://ccdb-test.cern.ch:8080/PHS/BadMap/Ped/1/73fe7593-d683-11ec-a015-c0a80209250c,http://ccdb-test.cern.ch:8080/MID/Calib/GlobalTriggerCrateConfig/1613573156894/d7ecd970-712e-11eb-82af-0aa1402f250c,http://ccdb-test.cern.ch:8080/IT3/Calib/Align/1/cf21e63c-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Calib/Data/1613573156864/d7e73420-712e-11eb-829b-0aa1402f250c,http://ccdb-test.cern.ch:8080/ITS/Align/Data/1613573156818/d7e1dcf0-712e-11eb-8283-0aa1402f250c,http://ccdb-test.cern.ch:8080/PHS/Calib/Mapping/1613573156900/d7ee11f0-712e-11eb-82b0-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_hvDriftUmon/1613573156865/d7e7d060-712e-11eb-829c-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Gaintbl_Uniform_FGAN0_2012-01/1/ca4479a4-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/TST/MO/FunctionalTestl1XcV/example/1629897680227/560b9640-05a7-11ec-a11d-200114580204,http://ccdb-test.cern.ch:8080/TPC/Config/HighVoltage/1613564633195/ff57bac0-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/MCH/Align/Baseline/1613573156889/d7ec6440-712e-11eb-82ab-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Config/HighVoltageStat/1613564633192/ff5793b0-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TOF/Calib/ParOnline/1613564634082/ffdf6150-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Mean_2/1646052154191/e6da2748-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb22_trkl-b0-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1_ptrg.r5037/1/2ba94ad0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/FT0/Calib/PulseShapes/1613573156910/d7ef2360-712e-11eb-82bc-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/Pulser/1656051775992/1632e031-f386-11ec-b836-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Mean_0/1646051868701/3caee8da-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1-pt100_ptrg.r5762/1/2b48db50-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/MO/MultiNodeRemoteTest30461/example2/1629896919339/9084ae80-05a5-11ec-a10f-200114580204,http://ccdb-test.cern.ch:8080/CPV/PhysicsRun/GainCalibData/946684800000/b2987f3a-d36d-11ec-8bf2-c0a80209250c,http://ccdb-test.cern.ch:8080/TST/MO/skeletonTask/example2/1629896883928/7b693d40-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/MID/Calib/RegionalTriggerBoardMasks/1613564635133/007fc000-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5765/1/2b5cb170-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/ZDC/Calib/InterCalibConfig/1550600800000/cfbca486-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Calib/PeakFinder/1613573156881/d7eadda0-712e-11eb-82a4-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5766/1/2abd8b40-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/CPV/Calib/Gains/1659884417458/a63e986e-1661-11ed-a464-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv2en-pt100_ptrg.r5764/1/2ad8b460-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/Calib/PHQ/1613573156886/d7ec3d30-712e-11eb-82ab-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv2en-pt100_ptrg.r5764/1/2ae25150-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/MO/QcTask/example2/1629898195048/88e6e280-05a8-11ec-a130-200114580204,http://ccdb-test.cern.ch:8080/CTP/Calib/Inputs/1656960306343/6c30f304-fbc9-11ec-9050-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Calib/Align/1/ceb12202-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_hvAnodeImon/1613573157211/d9af49f0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/Mapping/1613573156842/d7e36390-712e-11eb-8287-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_hvAnodeUmon/1613573156866/d7e81e80-712e-11eb-829c-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Calib/HVStatus/1651755029248/f14a416d-cc71-11ec-988e-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/HW/1613573156863/d7e73420-712e-11eb-829a-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_hvDriftImon/1613573156871/d7e8bac0-712e-11eb-829e-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Mean_3/1646052030150/9ce9f624-9893-11ec-8d5e-0aa14015250c,http://ccdb-test.cern.ch:8080/MCH/Calib/Neighbours/1613573157188/d9587760-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/Config/ClustererParam/1/45794039-d4af-11ec-8d4a-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofieN2/1613573156852/d7e58670-712e-11eb-8294-0aa1402f250c,http://ccdb-test.cern.ch:8080/CPV/PedestalRun/DeadChannels/1654542736364/947f40a6-e5cc-11ec-84f5-c0a80209250c,http://ccdb-test.cern.ch:8080/TPC/Calib/Noise/1639643743674/28a9283f-5e4b-11ec-9864-808d13fc250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_chamberStatus/1613573156851/d7e49c10-712e-11eb-828e-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/MO/MultiNodeLocalTest34502/example/1629896919318/9082d9c0-05a5-11ec-a10d-200114580204,http://ccdb-test.cern.ch:8080/TOF/Calib/RunParams/1613573156862/d7e86ca0-712e-11eb-829e-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/DCS/1613573276808/4f2ddf20-712f-11eb-82cf-0aa1402f250c,http://ccdb-test.cern.ch:8080/EMC/Calib/BadChannels/1613573156864/d7e73420-712e-11eb-829c-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Calib/DaqSig/1613573156886/d7ebef10-712e-11eb-82a9-0aa1402f250c,http://ccdb-test.cern.ch:8080/TST/MO/QYZTask/example/1629896877230/776b82c0-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5767/1/2b288210-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5767/1/2b719900-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/EMC/Calib/Pedestals/1613573156877/d7e9f340-712e-11eb-82a0-0aa1402f250c,http://ccdb-test.cern.ch:8080/ITS/Calib/VCASN/1656612120952/bdbc6ca8-f89e-11ec-8bc9-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Align/Data/1613573156885/d7eb04b0-712e-11eb-82a8-0aa1402f250c,http://ccdb-test.cern.ch:8080/EMC/Calib/RecoParam/1613573156877/d7e9cc30-712e-11eb-829f-0aa1402f250c,http://ccdb-test.cern.ch:8080/CTP/Calib/Align/1/f4cca637-ccbf-11ec-98da-c0a80209250c,http://ccdb-test.cern.ch:8080/TST/MO/XYZTask/example/1629896877229/776b82c0-05a5-11ec-a109-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b0-fs1e24-ht200-qs0e23s23e22-pidlhc11dv2en_ptrg.r5764/1/2b32bb40-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/CPV/Calib/GainPedestals/1613573156915/d7f0d110-712e-11eb-82c8-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Krypton_2015-02/1/c98c6366-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv2en-pt100_ptrg.r5764/1/2b7b8410-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Mean_1/1646051868701/3cb01e57-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Krypton_2018-01/1/c9fba6b5-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Mean_2/1646051868701/3cb1036d-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/MFT/Calib/1613573157063/d5bb25b0-9246-11eb-80d6-2a01cb0404fc,http://ccdb-test.cern.ch:8080/TPC/Calib/RecoParam/1613573156840/d7e2ee60-712e-11eb-8285-0aa1402f250c,http://ccdb-test.cern.ch:8080/EMC/Calib/FeeDCS/1653331846334/47b86903-dac9-11ec-becf-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb22_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1hn-pt100_ptrg.r5151/1/2bb3ab10-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TPC/Calib/Align/1/ce668283-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/ParOnlineDelay/1613564634068/ffde9e00-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TPC/Config/Temperature/1613564633197/ff582ff0-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TPC/Calib/AltroConfig/1613573156967/d806f120-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/XXX/XXX/2/7decf08e-d817-11ec-b68a-c0a80209250c,http://ccdb-test.cern.ch:8080/MCH/Align/Data/1613573156890/d7ec8b50-712e-11eb-82ac-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/BaselineCalibConfig/1550600800000/d46aa7d3-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/EMCDCS_ESTU/61579526400/4e0c0440-8ce2-11eb-bafe-2a010e0a09fb,http://ccdb-test.cern.ch:8080/ZDC/Calib/LaserCalib/1613573156908/d7eed540-712e-11eb-82b2-0aa1402f250c,http://ccdb-test.cern.ch:8080/CPV/Calib/Mapping/1613573156897/d7ed75b0-712e-11eb-82b0-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/TrkAttach/1613573156859/d7e5fba0-712e-11eb-829a-0aa1402f250c,http://ccdb-test.cern.ch:8080/FV0/Config/DCSDPconfig/1657560317920/6f395e9d-013e-11ed-b9e1-c0a80209250c,http://ccdb-test.cern.ch:8080/PHS/Calib/Pedestals/1/bb208fcb-d0a2-11ec-80f6-c0a80209250c,http://ccdb-test.cern.ch:8080/MFT/Calib/DeadMap/1654602151184/ea805f16-e656-11ec-87b9-c0a80209250c,http://ccdb-test.cern.ch:8080/FV0/Config/LookupTable/1635732445552/8b5379d8-55c2-11ec-8acf-2a010e0a0b16,http://ccdb-test.cern.ch:8080/ITS/Config/AlpideParam/1/d1d345c0-d4ae-11ec-8d4a-c0a80209250c,http://ccdb-test.cern.ch:8080/PHS/Calib/RecoParam/1613573156880/d7ea6870-712e-11eb-82a4-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/MBCalib/1613564635164/00847af0-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TPC/Calib/PedestalNoise/1656051216202/c85ec27c-f384-11ec-b836-c0a80209250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/ChMap/1613573156901/d7ee11f0-712e-11eb-82b1-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Calib/Config/1613573156857/d7e5d490-712e-11eb-829a-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/Pedestals/1613573156999/d821a510-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/CE/1655980180869/6409b34f-f2df-11ec-afd3-c0a80209250c,http://ccdb-test.cern.ch:8080/FT0/Calib/ChargeEqualization/1613573156909/d7eefc50-712e-11eb-82b7-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Krypton_2011-01/1/c846dd4d-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/EMC/Config/CalibParam/1546300800000/52781b60-144e-11ed-a0e9-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Config/ChannelScaleFactors/1546300800001/6d53e1f8-13c9-11ed-8a4a-c0a80209250c,http://ccdb-test.cern.ch:8080/FV0/LookUpTable/1633007527326/03534820-21f0-11ec-a23d-808d13fc250c,http://ccdb-test.cern.ch:8080/PHS/Calib/CalibParams/1644509343399/dd00e865-d06e-11ec-b606-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Config/DCSDPconfig/1647423150317/00359646-a50c-11ec-82bf-0aa1229c250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/Align/1/cee35485-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Sigma_2/1646052154191/e6de354c-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/TowerCalib/1550600800000/cb5e4b29-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b0-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en_ptrg.r5765/1/2b1a2a30-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TST/MO/XYZTask/example2/1629896877246/776da5a0-05a5-11ec-a10a-200114580204,http://ccdb-test.cern.ch:8080/FDD/Calib/PMTrends/1613564635243/009061d0-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TST/Override/1/496cce77-dfa6-11ec-923a-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Config/Latest/Sigma_6/1645012289847/c8126549-8f1e-11ec-9ff2-c08714f4250c,http://ccdb-test.cern.ch:8080/TOF/Calib/Problematic/1613573156862/d7e86ca0-712e-11eb-829d-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5p-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5570/1/2b85e450-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TOF/Calib/Pulser/1613564634088/ffe024a0-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb22_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1-pt100_ptrg.r5037/1/2b767b00-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/EMC/Calib/Mapping/1613573156865/d7e78240-712e-11eb-829c-0aa1402f250c,http://ccdb-test.cern.ch:8080/FDD/Calib/TimeSlewing/1613573156952/d7fae330-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/GLO/Calib/Collimators/1647345946177/3f43ec72-a458-11ec-81db-808d14b6250c,http://ccdb-test.cern.ch:8080/TRD/Calib/DCSDPs/1647605878244/7289809c-a6b5-11ec-868a-808d14b6250c,http://ccdb-test.cern.ch:8080/TST/MO/MultiNodeLocalTest34502/example2/1629896919338/9084ae80-05a5-11ec-a10e-200114580204,http://ccdb-test.cern.ch:8080/TRD/Calib/Krypton/1613573157093/d8b113d0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/PadGainFactor/1613564631914/fe946ac0-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TRD/Calib/LocalGainFactor/1/4adb8576-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofiePeakArea/1613573156853/d7e5ad80-712e-11eb-8298-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Config/DCSDPconfig/1650631687551/75770a62-c23a-11ec-884c-c0a80209250c,http://ccdb-test.cern.ch:8080/MID/Config/DCSDPconfig/1647261154382/d36b477d-a392-11ec-a41e-869e1ccf250c,http://ccdb-test.cern.ch:8080/TRD/Calib/trd_goofiePeakPos/1613573156853/d7e58670-712e-11eb-8295-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b2p-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5585/1/2ab8a940-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/CPV/Calib/Pedestals/1654542736364/950b6d2f-e5cc-11ec-84f5-c0a80209250c,http://ccdb-test.cern.ch:8080/FDD/Calib/Saturation/1613573156917/d7f257b0-712e-11eb-82cb-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/ChamberGainFactor/1613573156842/d7e36390-712e-11eb-8286-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/Config/DCSDPconfig/1633945985314/084594f0-2a79-11ec-9e6f-85295a4f250c,http://ccdb-test.cern.ch:8080/FDD/LookUpTable/1630431775310/dfe0f150-0a82-11ec-a525-b9cf5815250c,http://ccdb-test.cern.ch:8080/EMC/FeeDCS/1652109651005/6bef1756-cfa7-11ec-a067-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/LocalVdrift/1613573156851/d7e4ea30-712e-11eb-8290-0aa1402f250c,http://ccdb-test.cern.ch:8080/MCH/Calib/Config/1613573156897/d7edc3d0-712e-11eb-82b0-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test1/Mean_4/1646051868701/3cb1cd7a-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/TPC/Align/Data/1613573156817/d7dc1090-712e-11eb-8283-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Align/Data/1613573156879/d7ea6870-712e-11eb-82a1-0aa1402f250c,http://ccdb-test.cern.ch:8080/FDD/Calib/Thresholds/1613573156915/d7efe6b0-712e-11eb-82c8-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb26_trkl-b0-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en_ptrg.r5771/1/2b3ec930-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/FDD/Calib/ChannelTimeOffset/1546300800/9e832036-f6e0-11ec-89aa-c0a80209250c,http://ccdb-test.cern.ch:8080/TPC/Calib/IonTail/1613573157068/d88e2280-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/CPV/PedestalRun/FEEThresholds/1654542736364/94b9c92c-e5cc-11ec-84f5-c0a80209250c,http://ccdb-test.cern.ch:8080/FDD/Calib/PulseShapes/1613573156915/d7efe6b0-712e-11eb-82c7-0aa1402f250c,http://ccdb-test.cern.ch:8080/FDD/Config/DCSDPconfig/1657560638770/2e5c918e-013f-11ed-b9e3-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b0-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en_ptrg.r5766/1/2add6f50-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/FDD/Calib/LightYields/1613573156914/d7efbfa0-712e-11eb-82c6-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Calib/LocalT0/1613573156848/d7e47500-712e-11eb-828d-0aa1402f250c,http://ccdb-test.cern.ch:8080/EMC/Calib/Time/1613573156877/d7e9f340-712e-11eb-829f-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/TimeGain/0/34011532-9b4e-11ec-9479-2a0209080183,http://ccdb-test.cern.ch:8080/TPC/Config/FEEPad/0/ce172f10-9b09-11ec-9386-2a0209080183,http://ccdb-test.cern.ch:8080/ZDC/Calib/SaturationCalib/1613564635172/00858c60-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/MFT/Condition/CurrentAnalog/3/62cbf860-df7e-11eb-88b5-200102f801c1,http://ccdb-test.cern.ch:8080/PHS/Calib/GainPedestals/1613573156905/d7eefc50-712e-11eb-82b8-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/WaveformCalibConfig/1550600800000/d2157d58-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/FDD/Config/LookupTable/1654105278523/0afcffb2-e1d2-11ec-a2be-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Pedestals/Test0/Sigma_4/1646052154191/e6dee42d-9893-11ec-8d5e-0aa14014250c,http://ccdb-test.cern.ch:8080/FDD/Calib/ChargeEqualization/1613573156914/d7ef9890-712e-11eb-82c5-0aa1402f250c,http://ccdb-test.cern.ch:8080/FDD/Calib/PMGains/1613573156914/d7efbfa0-712e-11eb-82c7-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/Align/Data/1613573156848/d7e44df0-712e-11eb-828c-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Calib/ReadoutEfficiency/1613573156867/d7e84590-712e-11eb-829c-0aa1402f250c,http://ccdb-test.cern.ch:8080/MID/DCSconfig/1631259657/70082e40-120a-11ec-b9ab-4f590d6c250c,http://ccdb-test.cern.ch:8080/MCH/Calib/BPEVO/1613573156950/d7f9f8d0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/PHS/Calib/Trigger/1/2b919c70-4251-11ec-b6d2-5f54dcbf250c,http://ccdb-test.cern.ch:8080/GRP/Config/DCSDPconfig/1647010478528/2cb9e02b-a14b-11ec-a076-511cc1ec250c,http://ccdb-test.cern.ch:8080/FT0/Calib/Align/1/ceefe3bf-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/ITS/Calib/Align/1635322620830/7a49b00f-bc3f-11ec-9004-c0a80209250c,http://ccdb-test.cern.ch:8080/GLO/Calib/EnvVars/1647345948188/4025aae2-a458-11ec-81db-808d14b6250c,http://ccdb-test.cern.ch:8080/TRD/OnlineGainTables/Krypton_2011-02/1/c872a104-5455-11ec-a337-200114580204,http://ccdb-test.cern.ch:8080/MFT/Calib/NoiseMap/1650272759515/c3ab8fe2-bef6-11ec-9fbd-c0a80209250c,http://ccdb-test.cern.ch:8080/TPC/Calib/ClusterParam/1613573156828/d7e2c750-712e-11eb-8283-0aa1402f250c,http://ccdb-test.cern.ch:8080/PHS/Calib/TriggerMap/1659942762626/f2c632c5-16f0-11ed-a662-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Config/Preprocessor/1613573156879/d7ea4160-712e-11eb-82a0-0aa1402f250c,http://ccdb-test.cern.ch:8080/FT0/Calib/GlobalDelays/1613573156919/d7f47a90-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/ChannelCalib/0/1c0cd4d0-1615-11ec-a8d3-808d13fc250c,http://ccdb-test.cern.ch:8080/ZDC/Config/Module/1550600800000/bf85b066-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/BaselineCalib/1550600800000/d6ae5ff5-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/ConfigNoise/1613573156857/d7e5d490-712e-11eb-8298-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/PadTime0/1613573157041/d84f92e0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/TopologyGain/0/0abc5e34-a926-11ec-9048-200300ce7f10,http://ccdb-test.cern.ch:8080/MFT/Condition/DCSDPs/1658285963681/f63194e2-07d7-11ed-8753-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b0-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en_ptrg.r5767/1/2ac8fcf0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/FDD/Calib/RecoParam/1613573156915/d7f0f820-712e-11eb-82c8-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/Calib/Align/1635322620830/8a144d1f-bc3f-11ec-9005-c0a80209250c,http://ccdb-test.cern.ch:8080/MID/ElectronicsMasks/2679/a64ea788-af3a-11ec-9209-808d13fc250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5765/1/2b810250-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/MID/Calib/TriggerEfficiency/1613573156902/d7ee11f0-712e-11eb-82b2-0aa1402f250c,http://ccdb-test.cern.ch:8080/FDD/Calib/TimeDelays/1613573156915/d7f0aa00-712e-11eb-82c8-0aa1402f250c,http://ccdb-test.cern.ch:8080/MCH/Calib/Capacitances/1613564634424/002543f0-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/EMC/Calib/Align/1/cea4aa9c-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/GLO/GRP/BunchFilling/1632754085385/2a626fe0-1fa2-11ec-956b-200114580202,http://ccdb-test.cern.ch:8080/TPC/Calib/LaserTracks/1655980225223/7e5273cd-f2df-11ec-afe8-c0a80209250c,http://ccdb-test.cern.ch:8080/TPC/Config/GasComposition/1613564633190/ff571e80-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/MCH/HV/30/428fed80-1238-11ec-ba5a-808d14b6250c,http://ccdb-test.cern.ch:8080/MCH/Calib/Gains/1613564634625/0049bbe0-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/MCH/LV/33/429062b0-1238-11ec-ba5a-808d14b6250c,http://ccdb-test.cern.ch:8080/TPC/Calib/PadGainFull/0/fe5a6227-a468-11ec-8207-8d02f3c0250c,http://ccdb-test.cern.ch:8080/TRD/Calib/Align/1/ce731f63-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/TDCCalib/1550600800000/c94a9df1-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5767/1/2b6c8ff0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TRD/Calib/CalPadGain/1631311200000/b43ba300-e737-11ec-8ae9-c0a80209250c,http://ccdb-test.cern.ch:8080/GLO/Config/GRPLHCIFData/1649692021466/a0caca82-b9ae-11ec-863f-c0a80209250c,http://ccdb-test.cern.ch:8080/GLO/Param/MatLUT/0/48b173a0-2c69-11ec-8b38-2a010e0a09fb,http://ccdb-test.cern.ch:8080/TOF/Calib/CTPLatency/1613564633798/ffb3e480-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TOF/Calib/Align/1/ce7f62e7-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/MID/Masks/0/6789f887-93e8-11ec-b3e8-869e1c12250c,http://ccdb-test.cern.ch:8080/TRD/Calib/ChamberStatus/1/4a62e128-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/GRP/CTP/CTPtiming/1613573156926/d7f64f50-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/FDD/Align/Data/1613573156914/d7ef9890-712e-11eb-82c4-0aa1402f250c,http://ccdb-test.cern.ch:8080/FT0/Align/1640991600000/1f71c895-c578-11ec-bf73-c0a80209250c,http://ccdb-test.cern.ch:8080/FDD/Calib/Align/1/cf08be5f-bc3e-11ec-9002-c0a80209250c,http://ccdb-test.cern.ch:8080/CPV/PedestalRun/HighPedChannels/1654542736364/9461fac5-e5cc-11ec-84f5-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/CalVdriftExB/1/4a601e75-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/ChamberCalibrations/1/4a61a97c-d21c-11ec-894c-c0a80209250c,http://ccdb-test.cern.ch:8080/HMP/Config/Latest/Mean_3/1645012289847/c79048f2-8f1e-11ec-9ff2-c08714f4250c,http://ccdb-test.cern.ch:8080/MID/Calib/TriggerLut/1613564635149/00823100-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/GLO/Config/GeometryAligned/1635169451274/c943f4de-8cfb-11ec-9c69-25fc50f9250c,http://ccdb-test.cern.ch:8080/TOF/Calib/LHCphase/0/0a3c5246-51f5-11ec-9f6a-808d13fc250c,http://ccdb-test.cern.ch:8080/MFT/Config/AlpideParam/1/da3372e7-d4ae-11ec-8d4a-c0a80209250c,http://ccdb-test.cern.ch:8080/GRP/Calib/MeanVertex/1613573156917/d7f230a0-712e-11eb-82ca-0aa1402f250c,http://ccdb-test.cern.ch:8080/MFT/Calib/ClusterDictionary/1653192000000/c1d125c6-d92a-11ec-b9ac-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5n-fs1e24-ht200-qs0e23s23e22-pidlhc11dv3en-pt100_ptrg.r5766/1/2b52c660-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TPC/Calib/Parameters/1613573156840/d7e2c750-712e-11eb-8284-0aa1402f250c,http://ccdb-test.cern.ch:8080/MID/Calib/RegionalTriggerConfig/1613573156894/d7ecd970-712e-11eb-82b0-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Config/DCSDPconfig/1647010803664/ee7c66a6-a14b-11ec-a078-511cc1ec250c,http://ccdb-test.cern.ch:8080/PHS/Calib/Runbyrun/1659947360030/32e8c987-16f4-11ed-a674-c0a80209250c,http://ccdb-test.cern.ch:8080/ZDC/Config/Sim/1550600800000/c2c4b160-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/Diagnostic/1/3cec691f-b71b-11ec-b89d-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Config/Temperature/1613573156880/d7ea6870-712e-11eb-82a2-0aa1402f250c,http://ccdb-test.cern.ch:8080/EMC/Calib/SimParam/1613573156877/d7e9cc30-712e-11eb-829e-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Config/Latest/Mean_5/1645012289847/c7aa7049-8f1e-11ec-9ff2-c08714f4250c,http://ccdb-test.cern.ch:8080/TPC/Calib/VDriftTgl/1659945812077/f4ce5789-16f0-11ed-a662-c0a80209250c,http://ccdb-test.cern.ch:8080/TPC/Calib/IDC/GROUPINGPAR/1657228186400/21710de8-fe39-11ec-a120-c0a80209250c,http://ccdb-test.cern.ch:8080/MID/Calib/TriggerDCS/1613564635144/008146a0-711b-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/TPC/Calib/IDC/GROUPINGPAR/A/1659799828957/8c2799ba-15d0-11ed-a356-c0a80209250c,http://ccdb-test.cern.ch:8080/GRP/CTP/Aliases/1613573156917/d7f257b0-712e-11eb-82cc-0aa1402f250c,http://ccdb-test.cern.ch:8080/PHS/Calib/Time/1613573156884/d7eadda0-712e-11eb-82a6-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/GasComposition/1613573157150/d911f880-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/GRP/Calib/RecoParam/1613573156917/d7f230a0-712e-11eb-82cb-0aa1402f250c,http://ccdb-test.cern.ch:8080/GLO/Config/LHCIF/1647255751063/3e791883-a386-11ec-a3ed-808d14b6250c,http://ccdb-test.cern.ch:8080/FCT/Calib/Align/1/227b97ce-d043-11ec-b500-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/DCSconfig/1632255091910/1cf5d240-1b18-11ec-85cf-2a010e0a09fb,http://ccdb-test.cern.ch:8080/TOF/Calib/DCSDPs/1647262929295/f510ed70-a396-11ec-a42e-808d14b6250c,http://ccdb-test.cern.ch:8080/TRD/Calib/ChamberExB/1613573156842/d7e33c80-712e-11eb-8286-0aa1402f250c,http://ccdb-test.cern.ch:8080/GRP/CTP/LTUConfig/1613573156918/d7f2cce0-712e-11eb-82cd-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Calib/DeltaBCOffset/1613564633801/ffb459b0-711a-11eb-996f-200114580202,http://ccdb-test.cern.ch:8080/GLO/Config/GRPMagField/1651755029248/f180a91e-cc71-11ec-988e-c0a80209250c,http://ccdb-test.cern.ch:8080/FT0/Align/Data/1613573156909/d7eefc50-712e-11eb-82b6-0aa1402f250c,http://ccdb-test.cern.ch:8080/GRP/GRP/Data/1613573156915/d7f0f820-712e-11eb-82c9-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/TDCCorr/1550600800000/c71e4dde-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/GLO/Config/PVertexer/0/446a2cb0-17d3-11ec-9f26-2a010e0a09fb,http://ccdb-test.cern.ch:8080/MFT/Calib/Params/1613573156816/d7dc1090-712e-11eb-8281-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/IDC/GROUPINGPAR/C/1658937185037/50b9b56b-0ddd-11ed-86c0-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/FEELIGHT/1/1ca1dbb1-b71d-11ec-b8a5-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5n-fs1e24-ht200-qs0e24s24e23-pidlinear-pt100_ptrg.r5570/1/2ace2d10-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/TOF/Calib/LVStatus/1651755029248/f14ad2e3-cc71-11ec-988e-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/MonitoringData/1613573156848/d7e3ffd0-712e-11eb-828a-0aa1402f250c,http://ccdb-test.cern.ch:8080/GRP/Calib/LHCClockPhase/1613573276807/4f2ccdb0-712f-11eb-82cf-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/Pedestals/1613573156908/d7eed540-712e-11eb-82b3-0aa1402f250c,http://ccdb-test.cern.ch:8080/TRD/TrapConfig/cf_pg-fpnp32_zs-s16-deh_tb24_trkl-b5p-fs1e24-ht200-qs0e23s23e22-pidlhc11dv1-pt100_ptrg.r5762/1/2b622fb0-1c4c-11ec-8bdc-200114580204,http://ccdb-test.cern.ch:8080/HMP/Config/Latest/Sigma_3/1645012289847/c7de03aa-8f1e-11ec-9ff2-c08714f4250c,http://ccdb-test.cern.ch:8080/GRP/GRP/ECS/1634331102000/d6b92fd0-2df9-11ec-8fef-511cc1ec250c,http://ccdb-test.cern.ch:8080/MFT/Condition/Current/3/c091d5f0-e05f-11eb-8b28-200102f801c1,http://ccdb-test.cern.ch:8080/TRD/Calib/ChamberT0/1613573156843/d7e38aa0-712e-11eb-8288-0aa1402f250c,http://ccdb-test.cern.ch:8080/GRP/GRP/LHCData/1613573156947/d7f95c90-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/HighVoltage/1613573157058/d86f9e00-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/PadGainResidual/1635651688320/27687044-c575-11ec-bf64-c0a80209250c,http://ccdb-test.cern.ch:8080/TRD/Calib/ChamberVdrift/1613573156843/d7e38aa0-712e-11eb-8289-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/DCSconfig/1648798265061/906b6d3a-b18d-11ec-9cd1-c0a80209250c,http://ccdb-test.cern.ch:8080/GRP/CTP/Scalers/1613573156933/d7f712a0-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/GLO/Config/GRPECS/1634335785000/9b70ca90-2e04-11ec-8ffb-511cc1ec250c,http://ccdb-test.cern.ch:8080/GRP/CTP/Config/1613573156918/d7f2a5d0-712e-11eb-82cd-0aa1402f250c,http://ccdb-test.cern.ch:8080/MID/Calib/TriggerScalers/1613573156915/d7f0f820-712e-11eb-82ca-0aa1402f250c,http://ccdb-test.cern.ch:8080/HMP/Config/Latest/Mean_6/1645012289847/c7c49015-8f1e-11ec-9ff2-c08714f4250c,http://ccdb-test.cern.ch:8080/TPC/Calib/Pedestal/1639643743674/28a4ef61-5e4b-11ec-9864-808d13fc250c,http://ccdb-test.cern.ch:8080/HMP/Config/Latest/Sigma_5/1645012289847/c7f781c6-8f1e-11ec-9ff2-c08714f4250c,http://ccdb-test.cern.ch:8080/GLO/GRP/0/3f9e2220-1c67-11ec-8ca3-200114580202,http://ccdb-test.cern.ch:8080/TOF/Calib/FineSlewing/1613573157251/d9f2bb90-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/RecoConfigZDC/1550600800000/c4e593fa-1486-11ed-a0f2-c0a80209250c,http://ccdb-test.cern.ch:8080/TOF/Calib/Noise/1613573156870/d7e893b0-712e-11eb-829e-0aa1402f250c,http://ccdb-test.cern.ch:8080/ZDC/Calib/RecoParam/1613573156909/d7eed540-712e-11eb-82b4-0aa1402f250c,http://ccdb-test.cern.ch:8080/PHS/Align/Data/1613573156880/d7ea6870-712e-11eb-82a3-0aa1402f250c,http://ccdb-test.cern.ch:8080/TPC/Calib/PadNoise/1613573157003/d828d100-712e-11eb-82ce-0aa1402f250c,http://ccdb-test.cern.ch:8080/EMC/Calib/TimeCalibParams/1659945024949/866dfffe-16f0-11ed-a65e-c0a80209250c,http://ccdb-test.cern.ch:8080/EMC/Calib/Trigger/1613573156879/d7ea6870-712e-11eb-82a0-0aa1402f250c,http://ccdb-test.cern.ch:8080/TOF/Calib/ChannelCalib/1659942822088/f4c21d30-16f0-11ed-a662-c0a80209250c,http://ccdb-test.cern.ch:8080/TPC/Calib/IDC/1613573328714/7f5b17d0-712f-11eb-82d3-0aa1402f250c,http://ccdb-test.cern.ch:8080/EMC/Calib/Temperature/1653343868934/3f80c900-dae5-11ec-bf0f-c0a80209250c,http://ccdb-test.cern.ch:8080/FV0/Calibration/ChannelTimeOffset/1634670337602/8bc75650-310f-11ec-a4a0-c20cb928250c,http://ccdb-test.cern.ch:8080/FV0/Calib/RecoParam/1613573156912/d7ef7180-712e-11eb-82c1-0aa1402f250c,http://ccdb-test.cern.ch:8080/FV0/Calib/Saturation/1613573156914/d7efbfa0-712e-11eb-82c5-0aa1402f250c,http://ccdb-test.cern.ch:8080/FT0/Calibration/ChannelTimeOffset/1546300800/7d5d9f2e-d5bd-11ec-977d-c0a80209250c)";
/*
g++ -std=c++11 GigaTest.cpp -lpthread -lcurl -luv -o GigaTest && ./GigaTest
*/

// Ideas: call handleSocket for each socket to close them (mark with flag passed outside)

/*
TODO:

- add asynchronous closeLoop call

- check what can be free'd in destructor
- free things in finalize Download

- change name "checkGlobals"
- pooling threads only when they exist

- multiple uv loop threads

Information:

- Curl multi handle automatically reuses connections. Source: https://everything.curl.dev/libcurl/connectionreuse
- Keepalive for http is set to 118 seconds by default by CURL Source: https://stackoverflow.com/questions/60141625/libcurl-how-does-connection-keep-alive-work

*/

namespace o2 {
namespace ccdb {

CCDBDownloader::CCDBDownloader()
{
  // Preparing timer to be used by curl
  timeout = new uv_timer_t();
  timeout->data = this;
  uv_loop_init(&loop);
  uv_timer_init(&loop, timeout);

  // Preparing timer that will close multi handle and it's sockets some time after transfer completes
  socketTimoutTimer = new uv_timer_t();
  uv_timer_init(&loop, socketTimoutTimer);
  socketTimoutTimer->data = this;

  // Preparing curl handle
  initializeMultiHandle();  

  // Global timer
  // uv_loop runs only when there are active handles, this handle guarantees the loop won't close after finishing first batch of requests
  auto timerCheckQueueHandle = new uv_timer_t();
  timerCheckQueueHandle->data = this;
  uv_timer_init(&loop, timerCheckQueueHandle);
  uv_timer_start(timerCheckQueueHandle, checkGlobals, 100, 100);

  loopThread = new std::thread(&CCDBDownloader::asynchLoop, this);
}

void CCDBDownloader::initializeMultiHandle()
{
  multiHandleActive = true;
  curlMultiHandle = curl_multi_init();
  curl_multi_setopt(curlMultiHandle, CURLMOPT_SOCKETFUNCTION, handleSocket);
  auto socketData = new DataForSocket();
  socketData->curlm = curlMultiHandle;
  socketData->objPtr = this;
  curl_multi_setopt(curlMultiHandle, CURLMOPT_SOCKETDATA, socketData);
  curl_multi_setopt(curlMultiHandle, CURLMOPT_TIMERFUNCTION, startTimeout);
  curl_multi_setopt(curlMultiHandle, CURLMOPT_TIMERDATA, timeout);
  curl_multi_setopt(curlMultiHandle, CURLMOPT_MAX_TOTAL_CONNECTIONS, maxHandlesInUse);

}

void onUVClose(uv_handle_t* handle)
{
  if (handle != NULL)
  {
    delete handle;
  }
}

void closeHandles(uv_handle_t* handle, void* arg)
{
  if (!uv_is_closing(handle))
    uv_close(handle, onUVClose);
}

CCDBDownloader::~CCDBDownloader()
{
  for(auto socketTimerPair : socketTimerMap) {
    uv_timer_stop(socketTimerPair.second);
    uv_close((uv_handle_t*)socketTimerPair.second, onUVClose);
  }

  // Close async thread
  closeLoop = true;
  loopThread->join();

  delete loopThread;

  // Close and if any handles are running then signal to close, and run loop once to close them
  // This may take more then one iteration of loop - hence the "while"
  while (UV_EBUSY == uv_loop_close(&loop)) {
    closeLoop = false;
    uv_walk(&loop, closeHandles, NULL);
    uv_run(&loop, UV_RUN_ONCE);
  }
  curl_multi_cleanup(curlMultiHandle);
}

bool alienRedirect(CURL* handle)
{
  char *url = NULL;
  curl_easy_getinfo(handle, CURLINFO_REDIRECT_URL, &url);
  if (url)
  {
    std::string urlStr(url);
    if (urlStr.find("alien") == std::string::npos)
      return false;
  }
  return true;
}

void closePolls(uv_handle_t* handle, void* arg)
{
  if (handle->type == UV_POLL) {
    if (!uv_is_closing(handle)) {
      uv_close(handle, onUVClose);
    }
  }
}

void closeMultiHandle(uv_timer_t* handle) {
  auto AD = (CCDBDownloader*)handle->data;
  curl_multi_cleanup(AD->curlMultiHandle);
  AD->multiHandleActive = false;

  uv_walk(&AD->loop, closePolls, NULL);
  uv_timer_stop(handle);
}

void onTimeout(uv_timer_t *req)
{
  auto AD = (CCDBDownloader *)req->data;
  int running_handles;
  curl_multi_socket_action(AD->curlMultiHandle, CURL_SOCKET_TIMEOUT, 0,
                           &running_handles);
  AD->checkMultiInfo();
}

// Is used to react to polling file descriptors in poll_handle
// Calls handle_socket indirectly for further reading*
// If call is finished closes handle indirectly by check multi info
void curl_perform(uv_poll_t *req, int status, int events)
{
  int running_handles;
  int flags = 0;
  if (events & UV_READABLE)
    flags |= CURL_CSELECT_IN;
  if (events & UV_WRITABLE)
    flags |= CURL_CSELECT_OUT;

  auto context = (CCDBDownloader::curl_context_t *)req->data;
  
  curl_multi_socket_action(context->objPtr->curlMultiHandle, context->sockfd, flags,
                           &running_handles);
  context->objPtr->checkMultiInfo();
}

// Initializes a handle using a socket and passes it to context
CCDBDownloader::curl_context_t *CCDBDownloader::createCurlContext(curl_socket_t sockfd, CCDBDownloader *objPtr)
{
  curl_context_t *context;

  context = (curl_context_t *)malloc(sizeof(*context));
  context->objPtr = objPtr;
  context->sockfd = sockfd;

  uv_poll_init_socket(&objPtr->loop, &context->poll_handle, sockfd);
  context->poll_handle.data = context;

  return context;
}

// Frees data from curl handle inside uv_handle*
void CCDBDownloader::curlCloseCB(uv_handle_t *handle)
{
  curl_context_t *context = (curl_context_t *)handle->data;
  free(context);
}

// Makes an asynchronious call to free curl context*
void CCDBDownloader::destroyCurlContext(curl_context_t *context)
{
  uv_close((uv_handle_t *)&context->poll_handle, curlCloseCB);
}

void callbackWrappingFunction(void (*cbFun)(void*), void* data, bool* completionFlag)
{
  cbFun(data);
  *completionFlag = true;
}

void CCDBDownloader::finalizeDownload(CURL* easy_handle)
{
  handlesInUse--;
  char* done_url;
  curl_easy_getinfo(easy_handle, CURLINFO_EFFECTIVE_URL, &done_url);
  // printf("%s DONE\n", done_url);

  PerformData *data;
  curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &data);
  curl_multi_remove_handle(curlMultiHandle, easy_handle);

  long code;
  curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &code);

  if (code != 200) {
    if (code != 303 && code != 304)
    {
      std::cout << "Weird code returned: " << code << "\n";
    }
    else
    {
      if (!alienRedirect(easy_handle))
        std::cout << "Redirected to a different server than alien\n";
    }
  }
  // curl_easy_getinfo(easy_handle,  CURLINFO_RESPONSE_CODE, data->codeDestination);

  if (data->callback)
  {
    bool *cbFlag = (bool *)malloc(sizeof(bool));
    *cbFlag = false;
    auto cbThread = new std::thread(&callbackWrappingFunction, data->cbFun, data->cbData, cbFlag);
    threadFlagPairVector.emplace_back(cbThread, cbFlag);
  }

  // If no requests left then signal finished based on type of operation
  if (--(*data->requestsLeft) == 0)
  {
    switch (data->type)
    {
    case BLOCKING:
      data->cv->notify_all();
      break;
    case ASYNCHRONOUS:
      *data->completionFlag = true;
      break;
    case ASYNCHRONOUS_WITH_CALLBACK:
      *data->completionFlag = true;
      // TODO launch callback
      break;
    }
  }
  delete data;

  // Check if any handles are waiting in queue
  checkHandleQueue();

  // Calling timout starts a new download
  int running_handles;
  curl_multi_socket_action(curlMultiHandle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  checkMultiInfo();

  // If no running handles left then schedule multihandle to close
  if (running_handles == 0)
  {
    uv_timer_start(socketTimoutTimer, closeMultiHandle, socketTimoutMS, 0);
    socketTimoutTimerRunning = true;
  }
}

void CCDBDownloader::checkMultiInfo()
{
  CURLMsg *message;
  int pending;

  while ((message = curl_multi_info_read(curlMultiHandle, &pending)))
  {
    switch (message->msg)
    {
    case CURLMSG_DONE:
    {
      /* Do not use message data after calling curl_multi_remove_handle() and
        curl_easy_cleanup(). As per curl_multi_info_read() docs:
        "WARNING: The data the returned pointer points to will not survive
        calling curl_multi_cleanup, curl_multi_remove_handle or
        curl_easy_cleanup." */
      finalizeDownload(message->easy_handle);
      
    }
    break;

    default:
      fprintf(stderr, "CURLMSG default\n");
      break;
    }
  }
}

// Connects curl timer with uv timer
int CCDBDownloader::startTimeout(CURLM *multi, long timeout_ms, void *userp)
{
  auto timeout = (uv_timer_t *)userp;

  if (timeout_ms < 0)
  {
    uv_timer_stop(timeout);
  }
  else
  {
    if (timeout_ms == 0)
      timeout_ms = 1; // Calling onTimout when timeout = 0 could create an infinite loop                       
    uv_timer_start(timeout, onTimeout, timeout_ms, 0);
  }
  return 0;
}

void closeHandleTimerCallback(uv_timer_t* handle)
{
  auto data = (CCDBDownloader::DataForClosingSocket*)handle->data;
  auto AD = data->AD;
  auto sock = data->socket;

  if (AD->socketTimerMap.find(sock) != AD->socketTimerMap.end()) {
    std::cout << "Closing socket (timer)" << sock << "\n";
    uv_timer_stop(AD->socketTimerMap[sock]);
    AD->socketTimerMap.erase(sock);
    close(sock);
    return;
  }
  std::cout << "Socket not found " << sock << " (timer)\n";
}

// Is used to react to curl_multi_socket_action
// If INOUT then assigns socket to multi handle and starts polling file descriptors in poll_handle by callback
int handleSocket(CURL *easy, curl_socket_t s, int action, void *userp,
                                         void *socketp)
{
  auto socketData = (CCDBDownloader::DataForSocket *)userp;
  auto AD = (CCDBDownloader*)socketData->objPtr;
  CCDBDownloader::curl_context_t *curl_context;
  int events = 0;

  switch (action)
  {
  case CURL_POLL_IN:
  case CURL_POLL_OUT:
  case CURL_POLL_INOUT:

    // Create context associated with socket and create a poll for said socket
    curl_context = socketp ? (CCDBDownloader::curl_context_t *)socketp : AD->createCurlContext(s, socketData->objPtr);
    curl_multi_assign(socketData->curlm, s, (void *)curl_context);

    if (action != CURL_POLL_IN)
      events |= UV_WRITABLE;
    if (action != CURL_POLL_OUT)
      events |= UV_READABLE;

    if (AD->socketTimerMap.find(s) != AD->socketTimerMap.end()) {
      uv_timer_stop(AD->socketTimerMap[s]);
    }

    uv_poll_start(&curl_context->poll_handle, events, curl_perform);
    break;
  case CURL_POLL_REMOVE:
    if (socketp)
    {
      if (AD->socketTimerMap.find(s) != AD->socketTimerMap.end()) {
        uv_timer_start(AD->socketTimerMap[s], closeHandleTimerCallback, AD->socketTimoutMS, 0);
      }

      // Stop polling the socket, remove context assiciated with it. Socket will stay open until multi handle is closed.
      uv_poll_stop(&((CCDBDownloader::curl_context_t *)socketp)->poll_handle);
      AD->destroyCurlContext((CCDBDownloader::curl_context_t *)socketp);
      curl_multi_assign(socketData->curlm, s, NULL);
    }
    break;
  default:
    abort();
  }

  return 0;
}

void CCDBDownloader::checkHandleQueue()
{
  if (!multiHandleActive) {
    initializeMultiHandle();
  }

  // Lock access to handle queue
  handlesQueueLock.lock();
  if (handlesToBeAdded.size() > 0)
  {
    // Postpone closing sockets
    if (socketTimoutTimerRunning) {
      uv_timer_stop(socketTimoutTimer);
      socketTimoutTimerRunning = false;
    }

    // Add handles without going over the limit
    while(handlesToBeAdded.size() > 0 && handlesInUse < maxHandlesInUse) {
      curl_multi_add_handle(curlMultiHandle, handlesToBeAdded.front());
      handlesInUse++;
      handlesToBeAdded.erase(handlesToBeAdded.begin());
    }
  }
  handlesQueueLock.unlock();
}

void checkGlobals(uv_timer_t *handle)
{
  // Check for closing signal
  auto AD = (CCDBDownloader*)handle->data;
  if(AD->closeLoop) {
    uv_timer_stop(handle);
    uv_stop(&AD->loop);
  }

  // Join and erase threads that finished running callback functions
  for (int i = 0; i < AD->threadFlagPairVector.size(); i++)
  {
    if (*(AD->threadFlagPairVector[i].second))
    {
      AD->threadFlagPairVector[i].first->join();
      delete (AD->threadFlagPairVector[i].first);
      delete (AD->threadFlagPairVector[i].second);
      AD->threadFlagPairVector.erase(AD->threadFlagPairVector.begin() + i);
    }
  }
}

void CCDBDownloader::asynchLoop()
{
  uv_run(&loop, UV_RUN_DEFAULT);
}

std::vector<CURLcode>* CCDBDownloader::batchAsynchPerform(std::vector<CURL*> handleVector, bool *completionFlag)
{
  auto codeVector = new std::vector<CURLcode>(handleVector.size());
  size_t *requestsLeft = new size_t();
  *requestsLeft = handleVector.size();

  handlesQueueLock.lock();
  for(int i = 0; i < handleVector.size(); i++)
  {
    auto *data = new CCDBDownloader::PerformData();

    data->codeDestination = &(*codeVector)[i];
    data->requestsLeft = requestsLeft;
    data->completionFlag = completionFlag;
    data->type = ASYNCHRONOUS;

    curl_easy_setopt(handleVector[i], CURLOPT_PRIVATE, data);
    handlesToBeAdded.push_back(handleVector[i]);
  }
  handlesQueueLock.unlock();
  makeLoopCheckQueueAsync();
  return codeVector;
}

CURLcode CCDBDownloader::blockingPerform(CURL* handle)
{
  std::vector<CURL*> handleVector;
  handleVector.push_back(handle);
  return batchBlockingPerform(handleVector).back();
}

std::vector<CURLcode> CCDBDownloader::batchBlockingPerform(std::vector<CURL*> handleVector)
{
  std::condition_variable cv;
  std::mutex cv_m;
  std::unique_lock<std::mutex> lk(cv_m);

  std::vector<CURLcode> codeVector(handleVector.size());
  size_t requestsLeft = handleVector.size();

  handlesQueueLock.lock();
  for(int i = 0; i < handleVector.size(); i++)
  {
    auto *data = new CCDBDownloader::PerformData();
    data->codeDestination = &codeVector[i];
    data->cv = &cv;
    data->type = BLOCKING;
    data->requestsLeft = &requestsLeft;

    curl_easy_setopt(handleVector[i], CURLOPT_PRIVATE, data);
    handlesToBeAdded.push_back(handleVector[i]);
  }
  handlesQueueLock.unlock();
  makeLoopCheckQueueAsync();
  cv.wait(lk);
  return codeVector;
}

// TODO turn to batch asynch with callback
CURLcode *CCDBDownloader::asynchPerformWithCallback(CURL* handle, bool *completionFlag, void (*cbFun)(void*), void* cbData)
{
  auto data = new CCDBDownloader::PerformData();
  auto code = new CURLcode();
  data->completionFlag = completionFlag;
  data->codeDestination = code;

  data->cbFun = cbFun;
  data->cbData = cbData;
  data->callback = true;
  data->type = ASYNCHRONOUS_WITH_CALLBACK;

  curl_easy_setopt(handle, CURLOPT_PRIVATE, data);

  handlesQueueLock.lock();
  handlesToBeAdded.push_back(handle);
  handlesQueueLock.unlock();

  return code;
}

void asyncUVHandleCallback(uv_async_t *handle)
{
  auto AD = (CCDBDownloader*)handle->data;
  uv_close((uv_handle_t*)handle, onUVClose);
  // stop handle and free its memory
  AD->checkHandleQueue();
  // uv_check_t will delete be deleted in its callback
}

void CCDBDownloader::makeLoopCheckQueueAsync()
{
  auto asyncHandle = new uv_async_t();
  asyncHandle->data = this;
  uv_async_init(&loop, asyncHandle, asyncUVHandleCallback);
  uv_async_send(asyncHandle);
}

// ------------------------------ TESTING ------------------------------ 

std::string extractETAG(std::string headers)
{
  auto etagLine = headers.find("ETag");
  auto etagStart = headers.find("\"", etagLine)+1;
  auto etagEnd = headers.find("\"", etagStart+1);
  return headers.substr(etagStart, etagEnd - etagStart);
}

size_t writeToString(void *contents, size_t size, size_t nmemb, std::string *dst)
{
  char *conts = (char *)contents;
  for (int i = 0; i < nmemb; i++)
  {
    (*dst) += *(conts++);
  }
  return size * nmemb;
}

void cleanAllHandles(std::vector<CURL*> handles)
{
  for(auto handle : handles)
    curl_easy_cleanup(handle);
}

void closesocket_callback(void *clientp, curl_socket_t item)
{
  auto AD = (CCDBDownloader*)clientp;
  if (AD->socketTimerMap.find(item) != AD->socketTimerMap.end()) {
    uv_timer_stop(AD->socketTimerMap[item]);
    AD->socketTimerMap.erase(item);
    close(item);
  }
}

curl_socket_t opensocket_callback(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
  auto AD = (CCDBDownloader*)clientp;
  auto sock = socket(address->family, address->socktype, address->protocol);

  AD->socketTimerMap[sock] = new uv_timer_t();
  uv_timer_init(&AD->loop, AD->socketTimerMap[sock]);

  auto data = new CCDBDownloader::DataForClosingSocket();
  data->AD = AD;
  data->socket = sock;
  AD->socketTimerMap[sock]->data = data;

  return sock;
}

void setHandleOptions(CURL* handle, std::string* dst, std::string* headers, std::string* path, CCDBDownloader* AD)
{
  if (AD) {
    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETFUNCTION, closesocket_callback);
    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETDATA, AD);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, AD);
  }

  curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, writeToString);
  curl_easy_setopt(handle, CURLOPT_HEADERDATA, headers);

  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, dst);
  curl_easy_setopt(handle, CURLOPT_URL, path->c_str());

  // if (aliceServer)
  //   api->curlSetSSLOptions(handle);
}

void setHandleOptionsForValidity(CURL* handle, std::string* dst, std::string* url, std::string* etag, CCDBDownloader* AD)
{

  if (AD) {
    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETFUNCTION, closesocket_callback);
    curl_easy_setopt(handle, CURLOPT_CLOSESOCKETDATA, AD);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETFUNCTION, opensocket_callback);
    curl_easy_setopt(handle, CURLOPT_OPENSOCKETDATA, AD);
  }
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, dst);
  curl_easy_setopt(handle, CURLOPT_URL, url->c_str());

  // if (aliceServer)
  //   api->curlSetSSLOptions(handle);

  std::string etagHeader = "If-None-Match: \"" + *etag + "\"";
  struct curl_slist *curlHeaders = nullptr;
  curlHeaders = curl_slist_append(curlHeaders, etagHeader.c_str());
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, curlHeaders);
}

std::vector<std::string> createPathsFromCS()
{
  std::vector<std::string> vec;

  std::string* pathsCS;
  if (aliceServer) {
    // pathsCS = &alicePathsCS;
  } else {
    // pathsCS = &ccdbPathsCS;
  }

  std::string tmp;
  for(int i = 0; i < pathsCS->size(); i++) {
    if ((*pathsCS)[i] == ',') {
      vec.push_back(tmp);
      tmp = "";
    } else {
      tmp += (*pathsCS)[i];
    }
  }
  vec.push_back(tmp);
  return vec;
}

std::vector<std::string> createEtagsFromCS()
{
  std::vector<std::string> vec;

  std::string* etagsCS;
  if (aliceServer) {
    // etagsCS = &aliceEtagsCS;
  } else {
    // etagsCS = &ccdbEtagsCS;
  }

  std::string tmp;
  for(int i = 0; i < etagsCS->size(); i++) {
    if ((*etagsCS)[i] == ',') {
      vec.push_back(tmp);
      tmp = "";
    } else {
      tmp += (*etagsCS)[i];
    }
  }
  vec.push_back(tmp);
  return vec;
}

int64_t blockingBatchTestSockets(int pathLimit = 0, bool printResult = false)
{
  // Preparing for downloading
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;
  
  CCDBDownloader AD;

  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Downloading objects
  std::cout << "Starting first batch\n";
  AD.batchBlockingPerform(handles);
  cleanAllHandles(handles);
  std::cout << "First batch completed\n";


  // Waiting for sockets to close
  sleep(5);

  std::vector<CURL*> handles2;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles2.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles2.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Downloading again
  std::cout << "Starting second batch\n";
  AD.batchBlockingPerform(handles2);
  cleanAllHandles(handles2);
  std::cout << "Second batch completed\n";

  return 0;
}

int64_t blockingBatchTest(int pathLimit = 0, bool printResult = false)
{
  // Preparing for downloading
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;
  
  CCDBDownloader AD;

  auto start = std::chrono::system_clock::now();
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Downloading objects
  AD.batchBlockingPerform(handles);

  cleanAllHandles(handles);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "BLOCKING BATCH TEST:  Download - " <<  difference << "ms.\n";

  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  return difference;
}

int64_t blockingBatchTestValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();

  CCDBDownloader AD;

  std::vector<std::string*> results;
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    auto handle = curl_easy_init();
    results.push_back(new std::string());
    handles.push_back(handle);
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], &AD);
  }

  // Checking objects validity
  AD.batchBlockingPerform(handles);

  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    long code;
    curl_easy_getinfo(handles[i], CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      char* url;
      curl_easy_getinfo(handles[i], CURLINFO_EFFECTIVE_URL, &url);
      std::cout << "INVALID CODE: " << code << ", URL: " << url << "\n";
    }
  }

  // Clean handles and print time
  cleanAllHandles(handles);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "BLOCKING BATCH TEST:  Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t asynchBatchTest(int pathLimit = 0, bool printResult = false)
{
  // Preparing urls and objects to write into
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;
  
  // Preparing downloader
  CCDBDownloader AD;

  auto start = std::chrono::system_clock::now();

  // Preparing handles
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    handles.push_back(curl_easy_init());
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handles.back(), results.back(), headers.back(), &paths[i], &AD);
  }

  // Performing requests
  bool requestFinished = false;
  AD.batchAsynchPerform(handles, &requestFinished);
  while (!requestFinished) sleep(0.05);

  // Cleanup and print time
  cleanAllHandles(handles);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "ASYNC BATCH TEST:     download - " << difference << "ms.\n";

  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  return difference;
}

int64_t asynchBatchTestValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();
  CCDBDownloader AD;

  std::vector<std::string*> results;
  std::vector<CURL*> handles;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    auto handle = curl_easy_init();
    results.push_back(new std::string());
    handles.push_back(handle);
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], &AD);
  }

  // Checking objects validity
  bool requestFinished = false;
  AD.batchAsynchPerform(handles, &requestFinished);
  while (!requestFinished) sleep(0.001);
  cleanAllHandles(handles);

  // Checking if objects did not change
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    long code;
    curl_easy_getinfo(handles[i], CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      std::cout << "INVALID CODE: " << code << "\n";
    }
  }

  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "ASYNC BATCH TEST:     Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t linearTest(int pathLimit = 0, bool printResult = false)
{
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;

  auto start = std::chrono::system_clock::now();
  CURL *handle = curl_easy_init();
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writeToString);

  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handle, results.back(), headers.back(), &paths[i], nullptr);
    curl_easy_perform(handle);

    long code;
    curl_easy_getinfo(handle, CURLINFO_HTTP_CODE, &code);
    if (code != 303) 
      std::cout << "Result different that 303. Received code: " << code << "\n";
  }

  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  curl_easy_cleanup(handle);
  
  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  if (printResult)
    std::cout << "LINEAR TEST:          download - " << difference << "ms.\n";
  return difference;
}

int64_t linearTestValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();

  std::vector<std::string*> results;
  CURL* handle = curl_easy_init();
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    results.push_back(new std::string());    
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], nullptr);
    
    curl_easy_perform(handle);
    long code;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      std::cout << "INVALID CODE: " << code << "\n";
    }
  }

  curl_easy_cleanup(handle);
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "LINEAR TEST:          Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t linearTestNoReuse(int pathLimit = 0, bool printResult = false)
{
  auto paths = createPathsFromCS();
  std::vector<std::string*> results;
  std::vector<std::string*> headers;
  std::unordered_map<std::string, std::string> urlETagMap;

  auto start = std::chrono::system_clock::now();

  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    CURL* handle = curl_easy_init();
    results.push_back(new std::string());
    headers.push_back(new std::string());
    setHandleOptions(handle, results.back(), headers.back(), &paths[i], nullptr);
    curl_easy_perform(handle);

    long code;
    curl_easy_getinfo(handle, CURLINFO_HTTP_CODE, &code);
    if (code != 303) 
      std::cout << "Result different that 303. Received code: " << code << "\n";

    curl_easy_cleanup(handle);
  }
  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "LINEAR TEST no reuse:      download - " <<  difference << "ms.\n";

  // Extracting etags
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    urlETagMap[paths[i]] = extractETAG(*headers[i]);
  }
  return difference;
}

int64_t linearTestNoReuseValidity(int pathLimit = 0, bool printResult = false)
{
  // Preparing for checking objects validity
  auto paths = createPathsFromCS();
  auto etags = createEtagsFromCS();
  auto start = std::chrono::system_clock::now();

  std::vector<std::string*> results;
  for (int i = 0; i < (pathLimit == 0 ? paths.size() : pathLimit); i++) {
    CURL* handle = curl_easy_init();
    results.push_back(new std::string());
    setHandleOptionsForValidity(handle, results.back(), &paths[i], &etags[i], nullptr);
    
    curl_easy_perform(handle);
    long code;
    curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &code);
    if (code != 304) {
      std::cout << "INVALID CODE: " << code << "\n";
    }
    curl_easy_cleanup(handle);
  }

  auto end = std::chrono::system_clock::now();
  auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  if (printResult)
    std::cout << "LINEAR NO REUSE TEST: Check validity - " <<  difference << "ms.\n";
  return difference;
}

int64_t countAverageTime(int64_t (*function)(int, bool), int arg, int repeats)
{
  int64_t sum = 0;
  for(int i = 0; i < repeats; i++) {
    sum += function(arg, false);
  }
  return sum / repeats;
}

void CCDBDownloader::smallTest()
{
  std::cout << "I work!\n";
}

// int main()
// void GigaTest(CcdbApi* api)
// {
//   std::cout << "Test will be conducted on ";
//   if (aliceServer) {
//     std::cout << "https://alice-ccdb.cern.ch\n";
//     api = api;
//     api->init("https://alice-ccdb.cern.ch");
//   } else {
//     std::cout << "http://ccdb-test.cern.ch:8080\n";
//   }

//   if (curl_global_init(CURL_GLOBAL_ALL))
//   {
//     fprintf(stderr, "Could not init curl\n");
//     return;
//   }

//   int testSize = 100; // max 185

//   if (testSize != 0)
//     std::cout << "-------------- Testing for " << testSize << " objects with " << CCDBDownloader::maxHandlesInUse << " parallel connections. -----------\n";
//   else
//     std::cout << "-------------- Testing for all objects with " << CCDBDownloader::maxHandlesInUse << " parallel connections. -----------\n";

//   int repeats = 10;

//   // Just checking for 303
//   // std::cout << "Benchmarking redirect time\n";
//   // std::cout << "Blocking perform: " << countAverageTime(blockingBatchTest, testSize, repeats) << "ms.\n";
//   // std::cout << "Async    perform: " << countAverageTime(asynchBatchTest, testSize, repeats) << "ms.\n";
//   // std::cout << "Single   handle : " << countAverageTime(linearTest, testSize, repeats) << "ms.\n";
//   // std::cout << "Single no reuse : " << countAverageTime(linearTestNoReuse, testSize, repeats) << "ms.\n";


//   // std::cout << "--------------------------------------------------------------------------------------------\n";

//   std::cout << "Benchmarking test validity times\n";
//   std::cout << "Blocking perform validity: " << countAverageTime(blockingBatchTestValidity, testSize, repeats) << "ms.\n";
//   // std::cout << "Async    perform validity: " << countAverageTime(asynchBatchTestValidity, testSize, repeats) << "ms.\n";
//   // std::cout << "Single   handle  validity: " << countAverageTime(linearTestValidity, testSize, repeats) << "ms.\n";
//   // std::cout << "Single no reuse  validity: " << countAverageTime(linearTestNoReuseValidity, testSize, repeats) << "ms.\n";

//   // blockingBatchTestSockets(0, false);

//   curl_global_cleanup();
//   return;
// }

}
}