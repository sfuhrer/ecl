/****************************************************************************
 *
 *   Copyright (c) 2015 Estimation and Control Library (ECL). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name ECL nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file ekf.h
 * Class for core functions for ekf attitude and position estimator.
 *
 * @author Roman Bast <bapstroman@gmail.com>
 * @author Paul Riseborough <p_riseborough@live.com.au>
 *
 */

#pragma once

#include "estimator_interface.h"

class Ekf : public EstimatorInterface
{
public:

	Ekf() = default;
	virtual ~Ekf() = default;

	// initialise variables to sane values (also interface class)
	bool init(uint64_t timestamp);

	// should be called every time new data is pushed into the filter
	bool update();

	// gets the innovations of velocity and position measurements
	// 0-2 vel, 3-5 pos
	void get_vel_pos_innov(float vel_pos_innov[6]);

	// gets the innovations for of the NE auxiliary velocity measurement
	void get_aux_vel_innov(float aux_vel_innov[2]);

	// gets the innovations of the earth magnetic field measurements
	void get_mag_innov(float mag_innov[3]);

	// gets the innovations of the heading measurement
	void get_heading_innov(float *heading_innov);

	// gets the innovation variances of velocity and position measurements
	// 0-2 vel, 3-5 pos
	void get_vel_pos_innov_var(float vel_pos_innov_var[6]);

	// gets the innovation variances of the earth magnetic field measurements
	void get_mag_innov_var(float mag_innov_var[3]);

	// gets the innovations of airspeed measurement
	void get_airspeed_innov(float *airspeed_innov);

	// gets the innovation variance of the airspeed measurement
	void get_airspeed_innov_var(float *airspeed_innov_var);

	// gets the innovations of synthetic sideslip measurement
	void get_beta_innov(float *beta_innov);

	// gets the innovation variance of the synthetic sideslip measurement
	void get_beta_innov_var(float *beta_innov_var);

	// gets the innovation variance of the heading measurement
	void get_heading_innov_var(float *heading_innov_var);

	// gets the innovation variance of the flow measurement
	void get_flow_innov_var(float flow_innov_var[2]);

	// gets the innovation of the flow measurement
	void get_flow_innov(float flow_innov[2]);

	// gets the innovation variance of the drag specific force measurement
	void get_drag_innov_var(float drag_innov_var[2]);

	// gets the innovation of the drag specific force measurement
	void get_drag_innov(float drag_innov[2]);

	// gets the innovation variance of the HAGL measurement
	void get_hagl_innov_var(float *hagl_innov_var);

	// gets the innovation of the HAGL measurement
	void get_hagl_innov(float *hagl_innov);

	// get the state vector at the delayed time horizon
	void get_state_delayed(float *state);

	// get the wind velocity in m/s
	void get_wind_velocity(float *wind);

	// get the wind velocity var
	void get_wind_velocity_var(float *wind_var);

	// get the true airspeed in m/s
	void get_true_airspeed(float *tas);

	// get the full covariance matrix
	matrix::SquareMatrix<float, 24> covariances() const { return matrix::SquareMatrix<float, _k_num_states>(P); }

	// get the diagonal elements of the covariance matrix
	matrix::Vector<float, 24> covariances_diagonal() const { return covariances().diag(); }

	// get the orientation (quaterion) covariances
	matrix::SquareMatrix<float, 4> orientation_covariances() const { return covariances().slice<4, 4>(0, 0); }

	// get the linear velocity covariances
	matrix::SquareMatrix<float, 3> velocity_covariances() const { return covariances().slice<3, 3>(4, 4); }

	// get the position covariances
	matrix::SquareMatrix<float, 3> position_covariances() const { return covariances().slice<3, 3>(7, 7); }

	// ask estimator for sensor data collection decision and do any preprocessing if required, returns true if not defined
	bool collect_gps(const gps_message &gps);

	bool collect_imu(const imuSample &imu);

	// get the ekf WGS-84 origin position and height and the system time it was last set
	// return true if the origin is valid
	bool get_ekf_origin(uint64_t *origin_time, map_projection_reference_s *origin_pos, float *origin_alt);

	// get the 1-sigma horizontal and vertical position uncertainty of the ekf WGS-84 position
	void get_ekf_gpos_accuracy(float *ekf_eph, float *ekf_epv);

	// get the 1-sigma horizontal and vertical position uncertainty of the ekf local position
	void get_ekf_lpos_accuracy(float *ekf_eph, float *ekf_epv);

	// get the 1-sigma horizontal and vertical velocity uncertainty
	void get_ekf_vel_accuracy(float *ekf_evh, float *ekf_evv);

	// get the vehicle control limits required by the estimator to keep within sensor limitations
	void get_ekf_ctrl_limits(float *vxy_max, float *vz_max, float *hagl_min, float *hagl_max);

	/*
	Reset all IMU bias states and covariances to initial alignment values.
	Use when the IMU sensor has changed.
	Returns true if reset performed, false if rejected due to less than 10 seconds lapsed since last reset.
	*/
	bool reset_imu_bias();

	void get_vel_var(Vector3f &vel_var);

	void get_pos_var(Vector3f &pos_var);

	// return an array containing the output predictor angular, velocity and position tracking
	// error magnitudes (rad), (m/sec), (m)
	void get_output_tracking_error(float error[3]);

	/*
	Returns  following IMU vibration metrics in the following array locations
	0 : Gyro delta angle coning metric = filtered length of (delta_angle x prev_delta_angle)
	1 : Gyro high frequency vibe = filtered length of (delta_angle - prev_delta_angle)
	2 : Accel high frequency vibe = filtered length of (delta_velocity - prev_delta_velocity)
	*/
	void get_imu_vibe_metrics(float vibe[3]);

	/*
	First argument returns GPS drift  metrics in the following array locations
	0 : Horizontal position drift rate (m/s)
	1 : Vertical position drift rate (m/s)
	2 : Filtered horizontal velocity (m/s)
	Second argument returns true when IMU movement is blocking the drift calculation
	Function returns true if the metrics have been updated and not returned previously by this function
	*/
	bool get_gps_drift_metrics(float drift[3], bool *blocked);

	// return true if the global position estimate is valid
	bool global_position_is_valid();

	// check if the EKF is dead reckoning horizontal velocity using inertial data only
	void update_deadreckoning_status();

	// return true if the terrain estimate is valid
	bool get_terrain_valid();

	// update terrain validity status
	void update_terrain_valid();

	// get the estimated terrain vertical position relative to the NED origin
	void get_terrain_vert_pos(float *ret);

	// get the terrain variance
	float get_terrain_var() const { return _terrain_var; }

	// get the accelerometer bias in m/s/s
	void get_accel_bias(float bias[3]);

	// get the gyroscope bias in rad/s
	void get_gyro_bias(float bias[3]);

	// get GPS check status
	void get_gps_check_status(uint16_t *val);

	// return the amount the local vertical position changed in the last reset and the number of reset events
	void get_posD_reset(float *delta, uint8_t *counter) {*delta = _state_reset_status.posD_change; *counter = _state_reset_status.posD_counter;}

	// return the amount the local vertical velocity changed in the last reset and the number of reset events
	void get_velD_reset(float *delta, uint8_t *counter) {*delta = _state_reset_status.velD_change; *counter = _state_reset_status.velD_counter;}

	// return the amount the local horizontal position changed in the last reset and the number of reset events
	void get_posNE_reset(float delta[2], uint8_t *counter)
	{
		memcpy(delta, &_state_reset_status.posNE_change._data[0], sizeof(_state_reset_status.posNE_change._data));
		*counter = _state_reset_status.posNE_counter;
	}

	// return the amount the local horizontal velocity changed in the last reset and the number of reset events
	void get_velNE_reset(float delta[2], uint8_t *counter)
	{
		memcpy(delta, &_state_reset_status.velNE_change._data[0], sizeof(_state_reset_status.velNE_change._data));
		*counter = _state_reset_status.velNE_counter;
	}

	// return the amount the quaternion has changed in the last reset and the number of reset events
	void get_quat_reset(float delta_quat[4], uint8_t *counter)
	{
		memcpy(delta_quat, &_state_reset_status.quat_change._data[0], sizeof(_state_reset_status.quat_change._data));
		*counter = _state_reset_status.quat_counter;
	}

	// get EKF innovation consistency check status information comprising of:
	// status - a bitmask integer containing the pass/fail status for each EKF measurement innovation consistency check
	// Innovation Test Ratios - these are the ratio of the innovation to the acceptance threshold.
	// A value > 1 indicates that the sensor measurement has exceeded the maximum acceptable level and has been rejected by the EKF
	// Where a measurement type is a vector quantity, eg magnetometer, GPS position, etc, the maximum value is returned.
	void get_innovation_test_status(uint16_t *status, float *mag, float *vel, float *pos, float *hgt, float *tas, float *hagl, float *beta);

	// return a bitmask integer that describes which state estimates can be used for flight control
	void get_ekf_soln_status(uint16_t *status);

	// return the quaternion defining the rotation from the EKF to the External Vision reference frame
	void get_ekf2ev_quaternion(float *quat);

	// use the latest IMU data at the current time horizon.
	Quatf calculate_quaternion() const;

private:

	static constexpr uint8_t _k_num_states{24};		///< number of EKF states

	struct {
		uint8_t velNE_counter;	///< number of horizontal position reset events (allow to wrap if count exceeds 255)
		uint8_t velD_counter;	///< number of vertical velocity reset events (allow to wrap if count exceeds 255)
		uint8_t posNE_counter;	///< number of horizontal position reset events (allow to wrap if count exceeds 255)
		uint8_t posD_counter;	///< number of vertical position reset events (allow to wrap if count exceeds 255)
		uint8_t quat_counter;	///< number of quaternion reset events (allow to wrap if count exceeds 255)
		Vector2f velNE_change;  ///< North East velocity change due to last reset (m)
		float velD_change;	///< Down velocity change due to last reset (m/sec)
		Vector2f posNE_change;	///< North, East position change due to last reset (m)
		float posD_change;	///< Down position change due to last reset (m)
		Quatf quat_change;	///< quaternion delta due to last reset - multiply pre-reset quaternion by this to get post-reset quaternion
	} _state_reset_status{};	///< reset event monitoring structure containing velocity, position, height and yaw reset information

	float _dt_ekf_avg{FILTER_UPDATE_PERIOD_S}; ///< average update rate of the ekf
	float _dt_update{0.01f}; ///< delta time since last ekf update. This time can be used for filters which run at the same rate as the Ekf::update() function. (sec)

	stateSample _state{};		///< state struct of the ekf running at the delayed time horizon

	bool _filter_initialised{false};	///< true when the EKF sttes and covariances been initialised
	bool _earth_rate_initialised{false};	///< true when we know the earth rotatin rate (requires GPS)

	bool _fuse_height{false};	///< true when baro height data should be fused
	bool _fuse_pos{false};		///< true when gps position data should be fused
	bool _fuse_hor_vel{false};	///< true when gps horizontal velocity measurement should be fused
	bool _fuse_vert_vel{false};	///< true when gps vertical velocity measurement should be fused
	bool _fuse_hor_vel_aux{false};	///< true when auxiliary horizontal velocity measurement should be fused

	float _posObsNoiseNE{0.0f};		///< 1-STD observation noise used for the fusion of NE position data (m)
	float _posInnovGateNE{1.0f};		///< Number of standard deviations used for the NE position fusion innovation consistency check

	Vector2f _velObsVarNE;		///< 1-STD observation noise variance used for the fusion of NE velocity data (m/sec)**2
	float _hvelInnovGate{1.0f};		///< Number of standard deviations used for the horizontal velocity fusion innovation consistency check

	// variables used when position data is being fused using a relative position odometry model
	bool _fuse_hpos_as_odom{false};		///< true when the NE position data is being fused using an odometry assumption
	Vector3f _pos_meas_prev;		///< previous value of NED position measurement fused using odometry assumption (m)
	Vector2f _hpos_pred_prev;		///< previous value of NE position state used by odometry fusion (m)
	bool _hpos_prev_available{false};	///< true when previous values of the estimate and measurement are available for use
	Vector3f _ev_rot_vec_filt;		///< filtered rotation vector defining the rotation from EKF to EV reference (rad)
	Dcmf _ev_rot_mat;			///< transformation matrix that rotates observations from the EV to the EKF navigation frame
	uint64_t _ev_rot_last_time_us{0};	///< previous time that the calculation of the ekf to ev rotation matrix was updated (uSec)

	// booleans true when fresh sensor data is available at the fusion time horizon
	bool _gps_data_ready{false};	///< true when new GPS data has fallen behind the fusion time horizon and is available to be fused
	bool _mag_data_ready{false};	///< true when new magnetometer data has fallen behind the fusion time horizon and is available to be fused
	bool _baro_data_ready{false};	///< true when new baro height data has fallen behind the fusion time horizon and is available to be fused
	bool _range_data_ready{false};	///< true when new range finder data has fallen behind the fusion time horizon and is available to be fused
	bool _flow_data_ready{false};	///< true when the leading edge of the optical flow integration period has fallen behind the fusion time horizon
	bool _ev_data_ready{false};	///< true when new external vision system data has fallen behind the fusion time horizon and is available to be fused
	bool _tas_data_ready{false};	///< true when new true airspeed data has fallen behind the fusion time horizon and is available to be fused

	uint64_t _time_last_fake_gps{0};	///< last time we faked GPS position measurements to constrain tilt errors during operation without external aiding (uSec)
	uint64_t _time_ins_deadreckon_start{0};	///< amount of time we have been doing inertial only deadreckoning (uSec)
	bool _using_synthetic_position{false};	///< true if we are using a synthetic position to constrain drift

	uint64_t _time_last_pos_fuse{0};	///< time the last fusion of horizontal position measurements was performed (uSec)
	uint64_t _time_last_delpos_fuse{0};	///< time the last fusion of incremental horizontal position measurements was performed (uSec)
	uint64_t _time_last_vel_fuse{0};	///< time the last fusion of velocity measurements was performed (uSec)
	uint64_t _time_last_hgt_fuse{0};	///< time the last fusion of height measurements was performed (uSec)
	uint64_t _time_last_of_fuse{0};		///< time the last fusion of optical flow measurements were performed (uSec)
	uint64_t _time_last_arsp_fuse{0};	///< time the last fusion of airspeed measurements were performed (uSec)
	uint64_t _time_last_beta_fuse{0};	///< time the last fusion of synthetic sideslip measurements were performed (uSec)
	uint64_t _time_last_rng_ready{0};	///< time the last range finder measurement was ready (uSec)
	Vector2f _last_known_posNE;		///< last known local NE position vector (m)
	float _imu_collection_time_adj{0.0f};	///< the amount of time the IMU collection needs to be advanced to meet the target set by FILTER_UPDATE_PERIOD_MS (sec)

	uint64_t _time_acc_bias_check{0};	///< last time the  accel bias check passed (uSec)
	uint64_t _delta_time_baro_us{0};	///< delta time between two consecutive delayed baro samples from the buffer (uSec)

	uint64_t _last_imu_bias_cov_reset_us{0};	///< time the last reset of IMU delta angle and velocity state covariances was performed (uSec)

	Vector3f _earth_rate_NED;	///< earth rotation vector (NED) in rad/s

	Dcmf _R_to_earth;	///< transformation matrix from body frame to earth frame from last EKF prediction

	// used by magnetometer fusion mode selection
	Vector2f _accel_lpf_NE;			///< Low pass filtered horizontal earth frame acceleration (m/sec**2)
	float _yaw_delta_ef{0.0f};		///< Recent change in yaw angle measured about the earth frame D axis (rad)
	float _yaw_rate_lpf_ef{0.0f};		///< Filtered angular rate about earth frame D axis (rad/sec)
	bool _mag_bias_observable{false};	///< true when there is enough rotation to make magnetometer bias errors observable
	bool _yaw_angle_observable{false};	///< true when there is enough horizontal acceleration to make yaw observable
	uint64_t _time_yaw_started{0};		///< last system time in usec that a yaw rotation manoeuvre was detected
	uint8_t _num_bad_flight_yaw_events{0};	///< number of times a bad heading has been detected in flight and required a yaw reset
	uint64_t _mag_use_not_inhibit_us{0};	///< last system time in usec before magnetometer use was inhibited
	bool _mag_use_inhibit{false};		///< true when magnetometer use is being inhibited
	bool _mag_use_inhibit_prev{false};	///< true when magnetometer use was being inhibited the previous frame
	bool _mag_inhibit_yaw_reset_req{false};	///< true when magnetometer inhibit has been active for long enough to require a yaw reset when conditions improve.
	float _last_static_yaw{0.0f};		///< last yaw angle recorded when on ground motion checks were passing (rad)
	bool _vehicle_at_rest_prev{false};	///< true when the vehicle was at rest the previous time the status was checked
	bool _mag_yaw_reset_req{false};		///< true when a reset of the yaw using the magnetometer data has been requested
	bool _mag_decl_cov_reset{false};	///< true after the fuseDeclination() function has been used to modify the earth field covariances after a magnetic field reset event.

	float P[_k_num_states][_k_num_states] {};	///< state covariance matrix

	float _vel_pos_innov[6] {};	///< NED velocity and position innovations: 0-2 vel (m/sec),  3-5 pos (m)
	float _vel_pos_innov_var[6] {};	///< NED velocity and position innovation variances: 0-2 vel ((m/sec)**2), 3-5 pos (m**2)
	float _aux_vel_innov[2] {};	///< NE auxiliary velocity innovations: (m/sec)

	float _mag_innov[3] {};		///< earth magnetic field innovations (Gauss)
	float _mag_innov_var[3] {};	///< earth magnetic field innovation variance (Gauss**2)

	float _airspeed_innov{0.0f};		///< airspeed measurement innovation (m/sec)
	float _airspeed_innov_var{0.0f};	///< airspeed measurement innovation variance ((m/sec)**2)

	float _beta_innov{0.0f};	///< synthetic sideslip measurement innovation (rad)
	float _beta_innov_var{0.0f};	///< synthetic sideslip measurement innovation variance (rad**2)

	float _drag_innov[2] {};	///< multirotor drag measurement innovation (m/sec**2)
	float _drag_innov_var[2] {};	///< multirotor drag measurement innovation variance ((m/sec**2)**2)

	float _heading_innov{0.0f};	///< heading measurement innovation (rad)
	float _heading_innov_var{0.0f};	///< heading measurement innovation variance (rad**2)

	// optical flow processing
	float _flow_innov[2] {};	///< flow measurement innovation (rad/sec)
	float _flow_innov_var[2] {};	///< flow innovation variance ((rad/sec)**2)
	Vector3f _flow_gyro_bias;	///< bias errors in optical flow sensor rate gyro outputs (rad/sec)
	Vector3f _imu_del_ang_of;	///< bias corrected delta angle measurements accumulated across the same time frame as the optical flow rates (rad)
	float _delta_time_of{0.0f};	///< time in sec that _imu_del_ang_of was accumulated over (sec)
	uint64_t _time_bad_motion_us{0};	///< last system time that on-ground motion exceeded limits (uSec)
	uint64_t _time_good_motion_us{0};	///< last system time that on-ground motion was within limits (uSec)
	bool _inhibit_flow_use{false};	///< true when use of optical flow and range finder is being inhibited
	Vector2f _flowRadXYcomp;	///< measured delta angle of the image about the X and Y body axes after removal of body rotation (rad), RH rotation is positive

	// output predictor states
	Vector3f _delta_angle_corr;	///< delta angle correction vector (rad)
	imuSample _imu_down_sampled{};	///< down sampled imu data (sensor rate -> filter update rate)
	Quatf _q_down_sampled;		///< down sampled quaternion (tracking delta angles between ekf update steps)
	Vector3f _vel_err_integ;	///< integral of velocity tracking error (m)
	Vector3f _pos_err_integ;	///< integral of position tracking error (m.s)
	float _output_tracking_error[3] {}; ///< contains the magnitude of the angle, velocity and position track errors (rad, m/s, m)

	// variables used for the GPS quality checks
	float _gpsDriftVelN{0.0f};		///< GPS north position derivative (m/sec)
	float _gpsDriftVelE{0.0f};		///< GPS east position derivative (m/sec)
	float _gps_drift_velD{0.0f};		///< GPS down position derivative (m/sec)
	float _gps_velD_diff_filt{0.0f};	///< GPS filtered Down velocity (m/sec)
	float _gps_velN_filt{0.0f};		///< GPS filtered North velocity (m/sec)
	float _gps_velE_filt{0.0f};		///< GPS filtered East velocity (m/sec)
	uint64_t _last_gps_fail_us{0};		///< last system time in usec that the GPS failed it's checks
	uint64_t _last_gps_pass_us{0};		///< last system time in usec that the GPS passed it's checks
	float _gps_error_norm{1.0f};		///< normalised gps error

	// Variables used to publish the WGS-84 location of the EKF local NED origin
	uint64_t _last_gps_origin_time_us{0};	///< time the origin was last set (uSec)
	float _gps_alt_ref{0.0f};		///< WGS-84 height (m)

	// Variables used to initialise the filter states
	uint32_t _hgt_counter{0};		///< number of height samples read during initialisation
	float _rng_filt_state{0.0f};		///< filtered height measurement (m)
	uint32_t _mag_counter{0};		///< number of magnetometer samples read during initialisation
	uint32_t _ev_counter{0};		///< number of external vision samples read during initialisation
	uint64_t _time_last_mag{0};		///< measurement time of last magnetomter sample (uSec)
	Vector3f _mag_filt_state;		///< filtered magnetometer measurement (Gauss)
	Vector3f _delVel_sum;			///< summed delta velocity (m/sec)
	float _hgt_sensor_offset{0.0f};		///< set as necessary if desired to maintain the same height after a height reset (m)
	float _baro_hgt_offset{0.0f};		///< baro height reading at the local NED origin (m)

	// Variables used to control activation of post takeoff functionality
	float _last_on_ground_posD{0.0f};	///< last vertical position when the in_air status was false (m)
	bool _flt_mag_align_converging{false};	///< true when the in-flight mag field post alignment convergence is being performd
	uint64_t _flt_mag_align_start_time{0};	///< time that inflight magnetic field alignment started (uSec)
	uint64_t _time_last_movement{0};	///< last system time that sufficient movement to use 3-axis magnetometer fusion was detected (uSec)
	float _saved_mag_bf_variance[4] {};	///< magnetic field state variances that have been saved for use at the next initialisation (Gauss**2)
	float _saved_mag_ef_covmat[2][2] {};    ///< NE magnetic field state covariance sub-matrix saved for use at the next initialisation (Gauss**2)
	bool _velpos_reset_request{false};	///< true when a large yaw error has been fixed and a velocity and position state reset is required

	gps_check_fail_status_u _gps_check_fail_status{};

	// variables used to inhibit accel bias learning
	bool _accel_bias_inhibit{false};	///< true when the accel bias learning is being inhibited
	Vector3f _accel_vec_filt{};		///< acceleration vector after application of a low pass filter (m/sec**2)
	float _accel_mag_filt{0.0f};	///< acceleration magnitude after application of a decaying envelope filter (rad/sec)
	float _ang_rate_mag_filt{0.0f};		///< angular rate magnitude after application of a decaying envelope filter (rad/sec)
	Vector3f _prev_dvel_bias_var;		///< saved delta velocity XYZ bias variances (m/sec)**2

	// Terrain height state estimation
	float _terrain_vpos{0.0f};		///< estimated vertical position of the terrain underneath the vehicle in local NED frame (m)
	float _terrain_var{1e4f};		///< variance of terrain position estimate (m**2)
	float _hagl_innov{0.0f};		///< innovation of the last height above terrain measurement (m)
	float _hagl_innov_var{0.0f};		///< innovation variance for the last height above terrain measurement (m**2)
	uint64_t _time_last_hagl_fuse{0};		///< last system time that the hagl measurement failed it's checks (uSec)
	bool _terrain_initialised{false};	///< true when the terrain estimator has been initialized
	float _sin_tilt_rng{0.0f};		///< sine of the range finder tilt rotation about the Y body axis
	float _cos_tilt_rng{0.0f};		///< cosine of the range finder tilt rotation about the Y body axis
	float _R_rng_to_earth_2_2{0.0f};	///< 2,2 element of the rotation matrix from sensor frame to earth frame
	bool _range_data_continuous{false};	///< true when we are receiving range finder data faster than a 2Hz average
	float _dt_last_range_update_filt_us{0.0f};	///< filtered value of the delta time elapsed since the last range measurement came into the filter (uSec)
	bool _hagl_valid{false};		///< true when the height above ground estimate is valid

	// height sensor fault status
	bool _baro_hgt_faulty{false};		///< true if valid baro data is unavailable for use
	bool _gps_hgt_faulty{false};		///< true if valid gps height data is unavailable for use
	bool _rng_hgt_faulty{false};		///< true if valid range finder height data is unavailable for use
	int _primary_hgt_source{VDIST_SENSOR_BARO};	///< specifies primary source of height data

	// imu fault status
	uint64_t _time_bad_vert_accel{0};	///< last time a bad vertical accel was detected (uSec)
	uint64_t _time_good_vert_accel{0};	///< last time a good vertical accel was detected (uSec)
	bool _bad_vert_accel_detected{false};	///< true when bad vertical accelerometer data has been detected

	// variables used to control range aid functionality
	bool _range_aid_mode_enabled{false};	///< true when range finder can be used in flight as the height reference instead of the primary height sensor
	bool _range_aid_mode_selected{false};	///< true when range finder is being used as the height reference instead of the primary height sensor

	// variables used to check range finder validity data
	float _rng_stuck_min_val{0.0f};		///< minimum value for new rng measurement when being stuck
	float _rng_stuck_max_val{0.0f};		///< maximum value for new rng measurement when being stuck

	// update the real time complementary filter states. This includes the prediction
	// and the correction step
	void calculateOutputStates();

	// initialise filter states of both the delayed ekf and the real time complementary filter
	bool initialiseFilter(void);

	// initialise ekf covariance matrix
	void initialiseCovariance();

	// predict ekf state
	void predictState();

	// predict ekf covariance
	void predictCovariance();

	// ekf sequential fusion of magnetometer measurements
	void fuseMag();

	// fuse the first euler angle from either a 321 or 312 rotation sequence as the observation (currently measures yaw using the magnetometer)
	void fuseHeading();

	// fuse the yaw angle obtained from a dual antenna GPS unit
	void fuseGpsAntYaw();

	// reset the quaternions states using the yaw angle obtained from a dual antenna GPS unit
	// return true if the reset was successful
	bool resetGpsAntYaw();

	// fuse magnetometer declination measurement
	// argument passed in is the declination uncertainty in radians
	void fuseDeclination(float decl_sigma);

	// apply sensible limits to the declination and length of the NE mag field states estimates
	void limitDeclination();

	// fuse airspeed measurement
	void fuseAirspeed();

	// fuse synthetic zero sideslip measurement
	void fuseSideslip();

	// fuse body frame drag specific forces for multi-rotor wind estimation
	void fuseDrag();

	// fuse velocity and position measurements (also barometer height)
	void fuseVelPosHeight();

	// reset velocity states of the ekf
	bool resetVelocity();

	// fuse optical flow line of sight rate measurements
	void fuseOptFlow();

	// calculate optical flow body angular rate compensation
	// returns false if bias corrected body rate data is unavailable
	bool calcOptFlowBodyRateComp();

	// initialise the terrain vertical position estimator
	// return true if the initialisation is successful
	bool initHagl();

	// run the terrain estimator
	void runTerrainEstimator();

	// update the terrain vertical position estimate using a height above ground measurement from the range finder
	void fuseHagl();

	// reset the heading and magnetic field states using the declination and magnetometer/external vision measurements
	// return true if successful
	bool resetMagHeading(Vector3f &mag_init, bool increase_yaw_var = true, bool update_buffer=true);

	// Do a forced re-alignment of the yaw angle to align with the horizontal velocity vector from the GPS.
	// It is used to align the yaw angle after launch or takeoff for fixed wing vehicle.
	bool realignYawGPS();

	// Return the magnetic declination in radians to be used by the alignment and fusion processing
	float getMagDeclination();

	// reset position states of the ekf (only horizontal position)
	bool resetPosition();

	// reset height state of the ekf
	void resetHeight();

	// modify output filter to match the the EKF state at the fusion time horizon
	void alignOutputFilter();

	// update the estimated angular misalignment vector between the EV naigration frame and the EKF navigation frame
	// and update the rotation matrix which transforms EV navigation frame measurements into NED
	void calcExtVisRotMat();


	// reset the estimated angular misalignment vector between the EV naigration frame and the EKF navigation frame
	// and reset the rotation matrix which transforms EV navigation frame measurements into NED
	void resetExtVisRotMat();

	// limit the diagonal of the covariance matrix
	void fixCovarianceErrors();

	// make ekf covariance matrix symmetric between a nominated state indexe range
	void makeSymmetrical(float (&cov_mat)[_k_num_states][_k_num_states], uint8_t first, uint8_t last);

	// constrain the ekf states
	void constrainStates();

	// generic function which will perform a fusion step given a kalman gain K
	// and a scalar innovation value
	void fuse(float *K, float innovation);

	// calculate the earth rotation vector from a given latitude
	void calcEarthRateNED(Vector3f &omega, float lat_rad) const;

	// return true id the GPS quality is good enough to set an origin and start aiding
	bool gps_is_good(const gps_message &gps);

	// Control the filter fusion modes
	void controlFusionModes();

	// control fusion of external vision observations
	void controlExternalVisionFusion();

	// control fusion of optical flow observations
	void controlOpticalFlowFusion();

	// control fusion of GPS observations
	void controlGpsFusion();

	// control fusion of magnetometer observations
	void controlMagFusion();

	// control fusion of range finder observations
	void controlRangeFinderFusion();

	// control fusion of air data observations
	void controlAirDataFusion();

	// control fusion of synthetic sideslip observations
	void controlBetaFusion();

	// control fusion of multi-rotor drag specific force observations
	void controlDragFusion();

	// control fusion of pressure altitude observations
	void controlBaroFusion();

	// control fusion of velocity and position observations
	void controlVelPosFusion();

	// control fusion of auxiliary velocity observations
	void controlAuxVelFusion();

	// control for height sensor timeouts, sensor changes and state resets
	void controlHeightSensorTimeouts();

	// control for combined height fusion mode (implemented for switching between baro and range height)
	void controlHeightFusion();

	// determine if flight condition is suitable so use range finder instead of the primary height sensor
	void rangeAidConditionsMet();

	// check for "stuck" range finder measurements when rng was not valid for certain period
	void checkRangeDataValidity();

	// return the square of two floating point numbers - used in auto coded sections
	static constexpr float sq(float var) { return var * var; }

	// set control flags to use baro height
	void setControlBaroHeight();

	// set control flags to use range height
	void setControlRangeHeight();

	// set control flags to use GPS height
	void setControlGPSHeight();

	// set control flags to use external vision height
	void setControlEVHeight();

	// zero the specified range of rows in the state covariance matrix
	void zeroRows(float (&cov_mat)[_k_num_states][_k_num_states], uint8_t first, uint8_t last);

	// zero the specified range of columns in the state covariance matrix
	void zeroCols(float (&cov_mat)[_k_num_states][_k_num_states], uint8_t first, uint8_t last);

	// zero the specified range of off diagonals in the state covariance matrix
	void zeroOffDiag(float (&cov_mat)[_k_num_states][_k_num_states], uint8_t first, uint8_t last);

	// zero the specified range of off diagonals in the state covariance matrix
	// set the diagonals to the supplied value
	void setDiag(float (&cov_mat)[_k_num_states][_k_num_states], uint8_t first, uint8_t last, float variance);

	// calculate the measurement variance for the optical flow sensor
	float calcOptFlowMeasVar();

	// rotate quaternion covariances into variances for an equivalent rotation vector
	Vector3f calcRotVecVariances();

	// initialise the quaternion covariances using rotation vector variances
	void initialiseQuatCovariances(Vector3f &rot_vec_var);

	// perform a limited reset of the magnetic field state covariances
	void resetMagCovariance();

	// perform a limited reset of the wind state covariances
	void resetWindCovariance();

	// perform a reset of the wind states
	void resetWindStates();

	// check that the range finder data is continuous
	void checkRangeDataContinuity();

	// Increase the yaw error variance of the quaternions
	// Argument is additional yaw variance in rad**2
	void increaseQuatYawErrVariance(float yaw_variance);

	// save mag field state covariance data for re-use
	void save_mag_cov_data();

	// uncorrelate quaternion states from other states
	void uncorrelateQuatStates();

};
