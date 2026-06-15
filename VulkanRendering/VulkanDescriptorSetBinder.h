/******************************************************************************
This file is part of the Newcastle Vulkan Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*//////////////////////////////////////////////////////////////////////////////
#pragma once
namespace NCL::Rendering::Vulkan {
	/*
	DescriptorSetBinder: This helper class allows us to quickly bind multiple 
	descriptor sets to the same pipeline. Each set is bound using the command
	buffer at the point Bind is called, i.e. one at a time
	*/
	class DescriptorSetBinder {
	public:
		DescriptorSetBinder(vk::CommandBuffer inBuffer, 
			vk::PipelineLayout inLayout, 
			vk::PipelineBindPoint inBindpoint = vk::PipelineBindPoint::eGraphics) {
			assert(inBuffer);
			assert(inLayout);
			m_buffer = inBuffer;
			m_layout = inLayout;
			m_bindPoint = inBindpoint;
		}
		DescriptorSetBinder() {
			m_buffer = VK_NULL_HANDLE;
			m_layout = VK_NULL_HANDLE;
			m_bindPoint = vk::PipelineBindPoint::eGraphics;
		}
		~DescriptorSetBinder() {
		}

		DescriptorSetBinder& Bind(vk::DescriptorSet set, uint32_t slot){
			assert(set);
			m_buffer.bindDescriptorSets(m_bindPoint, m_layout, slot, 1, &set, 0, nullptr);
			return *this;
		}

	protected:
		vk::CommandBuffer		m_buffer;
		vk::PipelineLayout		m_layout;
		vk::PipelineBindPoint	m_bindPoint;
	};

	/*
	DescriptorSetMultiBinder: This helper class allows us to bind multiple
	descriptor sets to one or more pipelines. The sets are only bound at
	the point at which Commit is called, and will be done using as few
	individual bind calls as possible i.e adjacent sets will be bound
	together using a single vkBindDescriptorSets call. 

	No state is maintained relating to buffers or layouts, so it is safe
	to call Commit multiple times using different parameters. 
	*/
	class DescriptorSetMultiBinder {
		const static int MAX_SET_ARRAY = 16;
	public:
		DescriptorSetMultiBinder(int inFirstSlot = 0) {
			m_slotOffset	= inFirstSlot;
			m_firstSlot		= INT_MAX;
			m_lastSlot		= 0;
			m_slotCount		= 0;
		}
		~DescriptorSetMultiBinder() {
		}

		DescriptorSetMultiBinder& Bind(vk::DescriptorSet set, uint32_t slot) {
			assert(set);
			assert(slot < m_slotOffset + MAX_SET_ARRAY);
			m_sets[slot - m_slotOffset] = set;
			m_firstSlot = std::min(slot, m_firstSlot);
			m_lastSlot  = std::max(slot, m_lastSlot);
			m_slotCount++;
			return *this;
		}

		void Commit(vk::CommandBuffer buffer, vk::PipelineLayout m_layout, vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) {
			assert(buffer);
			assert(m_layout);

			if (m_lastSlot - m_firstSlot == m_slotCount-1) {
				//It's a contiguous set list!
				buffer.bindDescriptorSets(bindPoint, m_layout, m_firstSlot, m_slotCount, &m_sets[m_firstSlot - m_slotOffset], 0, nullptr);
				return;
			}
			int startingPoint = m_firstSlot;
			int subCount = 0;
			for (int i = m_firstSlot; i < MAX_SET_ARRAY; ++i) {
				if (subCount == 0 && !m_sets[i]) { //Skipping along a hole in the sets
					startingPoint++;
					continue;
				}
				if (m_sets[i]) {	//We've found a set!
					if (i == MAX_SET_ARRAY - 1) {
						buffer.bindDescriptorSets(bindPoint, m_layout, startingPoint, subCount, &m_sets[startingPoint - m_slotOffset], 0, nullptr);
						return;
					}
					subCount++;
					continue;
				}
				//Dump previously counted sets
				buffer.bindDescriptorSets(bindPoint, m_layout, startingPoint, subCount, &m_sets[startingPoint - m_slotOffset], 0, nullptr);
				startingPoint = i;
				subCount = 0;
			}
		}

	protected:
		vk::DescriptorSet	m_sets[MAX_SET_ARRAY];
		uint32_t			m_slotOffset;
		uint32_t			m_lastSlot;
		uint32_t			m_firstSlot;
		uint32_t			m_slotCount;
	};
}