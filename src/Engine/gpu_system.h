#pragma once

namespace gpu
{
	struct CreateInfo final
	{
		bool vSync{ false };
	};

	bool Init(const CreateInfo& createInfo);
	void Close();

	bool BeginFrame();
	void EndFrame();
} // namespace gpu