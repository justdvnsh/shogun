/*
 * This software is distributed under BSD 3-clause license (see LICENSE file).
 *
 * Authors: Sergey Lisitsyn, Heiko Strathmann, Chiyuan Zhang, Fernando Iglesias,
 *          Evan Shelhamer, Viktor Gal, Soeren Sonnenburg, Yuyu Zhang,
 *          Evangelos Anagnostopoulos
 */

#ifndef _LINEARMULTICLASSMACHINE_H___
#define _LINEARMULTICLASSMACHINE_H___

#include <shogun/lib/config.h>

#include <shogun/lib/common.h>
#include <shogun/features/DotFeatures.h>
#include <shogun/machine/LinearMachine.h>
#include <shogun/machine/MulticlassMachine.h>

namespace shogun
{

class DotFeatures;
class LinearMachine;
class MulticlassStrategy;

/** @brief generic linear multiclass machine */
class LinearMulticlassMachine : public MulticlassMachine
{
	public:
		/** default constructor  */
		LinearMulticlassMachine() : MulticlassMachine()
		{
			SG_ADD(&m_features, "m_features", "Feature object.");
		}

		/** standard constructor
		 * @param strategy multiclass strategy
		 * @param features features
		 * @param machine linear machine
		 * @param labs labels
		 */
		LinearMulticlassMachine(std::shared_ptr<MulticlassStrategy> strategy, std::shared_ptr<Features> features, std::shared_ptr<Machine> machine, std::shared_ptr<Labels> labs) :
			MulticlassMachine(strategy, machine,labs)
		{
			set_features(features->as<DotFeatures>());
			SG_ADD(&m_features, "m_features", "Feature object.");
		}

		/** destructor */
		~LinearMulticlassMachine() override
		{
		}

		/** get name */
		const char* get_name() const override
		{
			return "LinearMulticlassMachine";
		}

		/** set features
		 *
		 * @param f features
		 */
		void set_features(std::shared_ptr<DotFeatures> f)
		{
			m_features = f;
		}

		/** get features
		 *
		 * @return features
		 */
		std::shared_ptr<DotFeatures> get_features() const
		{
			return m_features;
		}

	protected:

		bool train_machine(std::shared_ptr<Features> data) override
		{
			m_features = data->as<DotFeatures>();
			require(m_multiclass_strategy, "Multiclass strategy not set");
			int32_t num_classes = m_labels->as<MulticlassLabels>()->get_num_classes();
   			m_multiclass_strategy->set_num_classes(num_classes);

			m_machines.clear();
			auto train_labels = std::make_shared<BinaryLabels>(get_num_rhs_vectors());

			m_multiclass_strategy->train_start(
				multiclass_labels(m_labels), train_labels);
			while (m_multiclass_strategy->train_has_more())
			{
				SGVector<index_t> subset=m_multiclass_strategy->train_prepare_next();
				if (subset.vlen)
				{
					train_labels->add_subset(subset);
					add_machine_subset(subset);
				}

				m_machine->train(data, train_labels);
				m_machines.push_back(get_machine_from_trained(m_machine));

				if (subset.vlen)
				{
					train_labels->remove_subset();
					remove_machine_subset();
				}
			}

			m_multiclass_strategy->train_stop();


			return true;
		}
		/** init machine for train with setting features */
		bool init_machine_for_train(std::shared_ptr<Features> data) override
		{
			if (!m_machine)
				error("No machine given in Multiclass constructor");

			if (data)
				set_features(data->as<DotFeatures>());

			return true;
		}

		/** init machines for applying with setting features */
		bool init_machines_for_apply(std::shared_ptr<Features> data) override
		{
			if (data)
				set_features(data->as<DotFeatures>());

			return true;
		}

		/** check features availability */
		bool is_ready() override
		{
			if (m_features)
				return true;

			return false;
		}

		/** construct linear machine from given linear machine */
		std::shared_ptr<Machine> get_machine_from_trained(std::shared_ptr<Machine> machine) const override
		{
			return machine->clone(ParameterProperties::MODEL)->as<LinearMachine>();
		}

		/** get number of rhs feature vectors */
		int32_t get_num_rhs_vectors() const override
		{
			return m_features->get_num_vectors();
		}

		/** set subset to the features of the machine, deletes old one
		 *
		 * @param subset subset instance to set
		 */
		void add_machine_subset(SGVector<index_t> subset) override
		{
			/* changing the subset structure to use subset stacks. This might
			 * have to be revised. Heiko Strathmann */
			m_features->add_subset(subset);
		}

		/** deletes any subset set to the features of the machine */
		void remove_machine_subset() override
		{
			/* changing the subset structure to use subset stacks. This might
			 * have to be revised. Heiko Strathmann */
			m_features->remove_subset();
		}

	protected:

		/** features */
		std::shared_ptr<DotFeatures> m_features;
};
}
#endif
