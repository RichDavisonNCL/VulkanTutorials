#pragma once

namespace NCL::Rendering {
	class Buffer {//Opaque GPU Buffer Type
	public:
		Buffer() {
			assetID = 0;
		}
		virtual ~Buffer() {

		}

		uint32_t GetAssetID() const {
			return assetID;
		}

		void SetAssetID(uint32_t newID) {
			assetID = newID;
		}
	protected:
		uint32_t assetID;
	};
}
