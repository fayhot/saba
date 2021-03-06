﻿//
// Copyright(c) 2016-2017 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#include "VMDAnimation.h"

#include <Saba/Base/Log.h>

#include <algorithm>
#include <iterator>
#include <map>
#include <glm/gtc/matrix_transform.hpp>

namespace saba
{

	float VMDBezier::EvalX(float t) const
	{
		float t2 = t * t;
		float t3 = t2 * t;
		float it = 1.0f - t;
		float it2 = it * it;
		float it3 = it2 * it;
		float x[4] = {
			m_cp[0].x,
			m_cp[1].x,
			m_cp[2].x,
			m_cp[3].x,
		};

		return t3 * x[3] + 3 * t2 * it * x[2] + 3 * t * it2 * x[1] + it3 * x[0];
	}

	float VMDBezier::EvalY(float t) const
	{
		float t2 = t * t;
		float t3 = t2 * t;
		float it = 1.0f - t;
		float it2 = it * it;
		float it3 = it2 * it;
		float y[4] = {
			m_cp[0].y,
			m_cp[1].y,
			m_cp[2].y,
			m_cp[3].y,
		};

		return t3 * y[3] + 3 * t2 * it * y[2] + 3 * t * it2 * y[1] + it3 * y[0];
	}

	namespace
	{
		float FindBezierX(const VMDBezier& b, float time)
		{
			const float e = 0.00001f;
			float start = 0.0f;
			float stop = 1.0f;
			float t = 0.5f;
			float x = b.EvalX(t);
			while (std::abs(time - x) > e)
			{
				if (time < x)
				{
					stop = t;
				}
				else
				{
					start = t;
				}
				t = (stop + start) * 0.5f;
				x = b.EvalX(t);
			}

			return t;
		}
	}

	glm::vec2 VMDBezier::Eval(float t) const
	{
		return glm::vec2(EvalX(t), EvalY(t));
	}

	VMDNodeController::VMDNodeController()
		: m_node(nullptr)
	{
	}

	void VMDNodeController::SetNode(MMDNode * node)
	{
		m_node = node;
	}

	void VMDNodeController::Evaluate(float t)
	{
		SABA_ASSERT(m_node != nullptr);
		if (m_node == nullptr)
		{
			return;
		}
		if (m_keys.empty())
		{
			m_node->SetAnimationTranslate(glm::vec3(0));
			m_node->SetAnimationRotate(glm::quat(1, 0, 0, 0));
			return;
		}

		auto boundIt = std::upper_bound(
			std::begin(m_keys),
			std::end(m_keys),
			t,
			[](float lhs, const KeyType& rhs) { return lhs < rhs.m_time; }
		);
		glm::vec3 vt;
		glm::quat q;
		if (boundIt == std::end(m_keys))
		{
			vt = m_keys[m_keys.size() - 1].m_translate;
			q = m_keys[m_keys.size() - 1].m_rotate;
		}
		else
		{
			vt = (*boundIt).m_translate;
			q = (*boundIt).m_rotate;
			if (boundIt != std::begin(m_keys))
			{
				const auto& key0 = *(boundIt - 1);
				const auto& key1 = *boundIt;

				float timeRange = key1.m_time - key0.m_time;
				float time = (t - key0.m_time) / timeRange;
				float tx_x = FindBezierX(key0.m_txBezier, time);
				float ty_x = FindBezierX(key0.m_tyBezier, time);
				float tz_x = FindBezierX(key0.m_tzBezier, time);
				float rot_x = FindBezierX(key0.m_rotBezier, time);
				float tx_y = key0.m_txBezier.EvalY(tx_x);
				float ty_y = key0.m_txBezier.EvalY(ty_x);
				float tz_y = key0.m_txBezier.EvalY(tz_x);
				float rot_y = key0.m_txBezier.EvalY(rot_x);

				glm::vec3 dt = key1.m_translate - key0.m_translate;
				vt = dt * glm::vec3(tx_y, ty_y, tz_y) + key0.m_translate;
				q = glm::slerp(key0.m_rotate, key1.m_rotate, rot_y);
			}
		}

		m_node->SetAnimationRotate(q);
		m_node->SetAnimationTranslate(vt);
	}

	void VMDNodeController::SortKeys()
	{
		std::sort(
			std::begin(m_keys),
			std::end(m_keys),
			[](const KeyType& a, const KeyType& b) { return a.m_time < b.m_time; }
		);
	}

	bool VMDAnimation::Create(std::shared_ptr<MMDModel> model)
	{
		m_model = model;
		return true;
	}

	bool VMDAnimation::Add(const VMDFile & vmd)
	{
		std::map<std::string, NodeControllerPtr> nodeCtrlMap;
		for (auto& nodeCtrl : m_nodeControllers)
		{
			std::string name = nodeCtrl->GetNode()->GetName();
			nodeCtrlMap.emplace(std::make_pair(name, std::move(nodeCtrl)));
		}
		m_nodeControllers.clear();
		for (const auto& motion : vmd.m_motions)
		{
			std::string nodeName = motion.m_boneName.ToUtf8String();
			auto findIt = nodeCtrlMap.find(nodeName);
			VMDNodeController* nodeCtrl = nullptr;
			if (findIt == std::end(nodeCtrlMap))
			{
				auto node = m_model->GetNodeManager()->GetMMDNode(nodeName);
				if (node != nullptr)
				{
					auto val = std::make_pair(
						nodeName,
						std::make_unique<VMDNodeController>()
					);
					nodeCtrl = val.second.get();
					nodeCtrl->SetNode(node);
					nodeCtrlMap.emplace(std::move(val));
				}
			}
			else
			{
				nodeCtrl = (*findIt).second.get();
			}

			if (nodeCtrl != nullptr)
			{
				VMDNodeAnimationKey key;
				key.m_time = (float)motion.m_frame;
				key.Set(motion);
				nodeCtrl->AddKey(key);
			}
		}
		m_nodeControllers.reserve(nodeCtrlMap.size());
		for (auto& pair : nodeCtrlMap)
		{
			pair.second->SortKeys();
			m_nodeControllers.emplace_back(std::move(pair.second));
		}
		nodeCtrlMap.clear();

		std::map<std::string, IKControllerPtr> ikCtrlMap;
		for (auto& ikCtrl : m_ikControllers)
		{
			std::string name = ikCtrl->GetIkSolver()->GetName();
			ikCtrlMap.emplace(std::make_pair(name, std::move(ikCtrl)));
		}
		m_ikControllers.clear();
		for (const auto& ik : vmd.m_iks)
		{
			for (const auto& ikInfo : ik.m_ikInfos)
			{
				std::string ikName = ikInfo.m_name.ToUtf8String();
				auto findIt = ikCtrlMap.find(ikName);
				VMDIKController* ikCtrl = nullptr;
				if (findIt == std::end(ikCtrlMap))
				{
					auto* ikSolver = m_model->GetIKManager()->GetMMDIKSolver(ikName);
					if (ikSolver != nullptr)
					{
						auto val = std::make_pair(
							ikName,
							std::make_unique<VMDIKController>()
						);
						ikCtrl = val.second.get();
						ikCtrl->SetIKSolver(ikSolver);
						ikCtrlMap.emplace(std::move(val));
					}
				}
				else
				{
					ikCtrl = (*findIt).second.get();
				}

				if (ikCtrl != nullptr)
				{
					VMDIKAnimationKey key;
					key.m_time = (float)ik.m_frame;
					key.m_enable = ikInfo.m_enable != 0;
					ikCtrl->AddKey(key);
				}
			}
		}
		m_ikControllers.reserve(ikCtrlMap.size());
		for (auto& pair : ikCtrlMap)
		{
			pair.second->SortKeys();
			m_ikControllers.emplace_back(std::move(pair.second));
		}
		ikCtrlMap.clear();

		std::map<std::string, MorphControllerPtr> morphCtrlMap;
		for (auto& bsCtrl : m_blendShapeControllers)
		{
			std::string name = bsCtrl->GetMorph()->GetName();
			morphCtrlMap.emplace(std::make_pair(name, std::move(bsCtrl)));
		}
		m_blendShapeControllers.clear();
		for (const auto& morph : vmd.m_morphs)
		{
			std::string bsName = morph.m_blendShapeName.ToUtf8String();
			auto findIt = morphCtrlMap.find(bsName);
			VMDMorphController* bsCtrl = nullptr;
			if (findIt == std::end(morphCtrlMap))
			{
				auto* mmdBS = m_model->GetMorphManager()->GetMorph(bsName);
				if (mmdBS != nullptr)
				{
					auto val = std::make_pair(
						bsName,
						std::make_unique<VMDMorphController>()
					);
					bsCtrl = val.second.get();
					bsCtrl->SetBlendKeyShape(mmdBS);
					morphCtrlMap.emplace(std::move(val));
				}
			}
			else
			{
				bsCtrl = (*findIt).second.get();
			}

			if (bsCtrl != nullptr)
			{
				VMDMorphAnimationKey key;
				key.m_time = (float)morph.m_frame;
				key.m_weight = morph.m_weight;
				bsCtrl->AddKey(key);
			}
		}
		m_blendShapeControllers.reserve(morphCtrlMap.size());
		for (auto& pair : morphCtrlMap)
		{
			pair.second->SortKeys();
			m_blendShapeControllers.emplace_back(std::move(pair.second));
		}
		morphCtrlMap.clear();

		return true;
	}

	void VMDAnimation::Destroy()
	{
		m_model.reset();
		m_nodeControllers.clear();
	}

	void VMDAnimation::Evaluate(float t)
	{
		for (auto& nodeCtrl : m_nodeControllers)
		{
			nodeCtrl->Evaluate(t);
		}

		for (auto& ikCtrl : m_ikControllers)
		{
			ikCtrl->Evaluate(t);
		}

		for (auto& bsCtrl : m_blendShapeControllers)
		{
			bsCtrl->Evaluate(t);
		}
	}

	namespace
	{
		void SetVMDBezier(VMDBezier& bezier, const unsigned char* cp)
		{
			int x0 = cp[0];
			int y0 = cp[4];
			int x1 = cp[8];
			int y1 = cp[12];

			bezier.m_cp[0] = glm::vec2(0, 0);
			bezier.m_cp[1] = glm::vec2((float)x0 / 127.0f, (float)y0 / 127.0f);
			bezier.m_cp[2] = glm::vec2((float)x1 / 127.0f, (float)y1 / 127.0f);
			bezier.m_cp[3] = glm::vec2(1, 1);
		}
	} // namespace

	void VMDNodeAnimationKey::Set(const VMDMotion & motion)
	{
		m_time = (float)motion.m_frame;

		m_translate = motion.m_translate * glm::vec3(1, 1, -1);

		const glm::quat q = motion.m_quaternion;
		auto invZ = glm::mat3(glm::scale(glm::mat4(1), glm::vec3(1, 1, -1)));
		auto rot0 = glm::mat3_cast(q);
		auto rot1 = invZ * rot0 * invZ;
		m_rotate = glm::quat_cast(rot1);

		SetVMDBezier(m_txBezier, &motion.m_interpolation[0]);
		SetVMDBezier(m_tyBezier, &motion.m_interpolation[1]);
		SetVMDBezier(m_tzBezier, &motion.m_interpolation[2]);
		SetVMDBezier(m_rotBezier, &motion.m_interpolation[3]);
	}

	VMDIKController::VMDIKController()
		: m_ikSolver(nullptr)
	{
	}

	void VMDIKController::SetIKSolver(MMDIkSolver * ikSolver)
	{
		m_ikSolver = ikSolver;
	}

	void VMDIKController::Evaluate(float t)
	{
		if (m_ikSolver == nullptr)
		{
			return;
		}

		auto it = std::partition_point(
			std::begin(m_keys),
			std::end(m_keys),
			[t](const VMDIKAnimationKey& key) {return key.m_time <= t;}
		);
		bool enable = false;
		if (it == std::begin(m_keys))
		{
			enable = (*it).m_enable;
		}
		else
		{
			const auto& key = *(it - 1);
			enable = key.m_enable;
		}
		m_ikSolver->Enable(enable);
	}

	void VMDIKController::SortKeys()
	{
		std::sort(
			std::begin(m_keys),
			std::end(m_keys),
			[](const KeyType& a, const KeyType& b) { return a.m_time < b.m_time; }
		);
	}

	VMDMorphController::VMDMorphController()
		: m_morph(nullptr)
	{
	}

	void VMDMorphController::SetBlendKeyShape(MMDMorph * morph)
	{
		m_morph = morph;
	}

	void VMDMorphController::Evaluate(float t)
	{
		if (m_morph == nullptr)
		{
			return;
		}

		if (m_keys.empty())
		{
			return;
		}

		auto findIt = std::upper_bound(
			std::begin(m_keys),
			std::end(m_keys),
			t,
			[](float lhs, const KeyType& rhs) { return lhs < rhs.m_time; }
		);

		VMDMorphAnimationKey key;
		if (findIt == std::end(m_keys))
		{
			key = *(findIt - 1);
		}
		else
		{
			key = *findIt;
		}

		float weight = key.m_weight;

		if (findIt != std::begin(m_keys) && findIt != std::end(m_keys))
		{
			VMDMorphAnimationKey key0 = *(findIt - 1);
			VMDMorphAnimationKey key1 = *findIt;

			float timeRange = key1.m_time - key0.m_time;
			float time = (t - key0.m_time) / timeRange;
			weight = (key1.m_weight - key0.m_weight) * time + key0.m_weight;
		}

		m_morph->SetWeight(weight);
	}

	void VMDMorphController::SortKeys()
	{
		std::sort(
			std::begin(m_keys),
			std::end(m_keys),
			[](const KeyType& a, const KeyType& b) { return a.m_time < b.m_time; }
		);
	}

}
