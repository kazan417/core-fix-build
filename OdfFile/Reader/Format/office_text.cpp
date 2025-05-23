﻿/*
 * (c) Copyright Ascensio System SIA 2010-2023
 *
 * This program is a free software product. You can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License (AGPL)
 * version 3 as published by the Free Software Foundation. In accordance with
 * Section 7(a) of the GNU AGPL its Section 15 shall be amended to the effect
 * that Ascensio System SIA expressly excludes the warranty of non-infringement
 * of any third-party rights.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR  PURPOSE. For
 * details, see the GNU AGPL at: http://www.gnu.org/licenses/agpl-3.0.html
 *
 * You can contact Ascensio System SIA at 20A-6 Ernesta Birznieka-Upish
 * street, Riga, Latvia, EU, LV-1050.
 *
 * The  interactive user interfaces in modified source and object code versions
 * of the Program must display Appropriate Legal Notices, as required under
 * Section 5 of the GNU AGPL version 3.
 *
 * Pursuant to Section 7(b) of the License you must retain the original Product
 * logo when distributing the program. Pursuant to Section 7(e) we decline to
 * grant you any rights under trademark law for use of our trademarks.
 *
 * All the Product's GUI elements, including illustrations and icon sets, as
 * well as technical writing content are licensed under the terms of the
 * Creative Commons Attribution-ShareAlike 4.0 International. See the License
 * terms at http://creativecommons.org/licenses/by-sa/4.0/legalcode
 *
 */

#include "office_text.h"
#include "office_meta.h"

#include <xml/xmlchar.h>
#include <xml/utils.h>
#include "odf_document.h"

#include "serialize_elements.h"
#include "odfcontext.h"
#include "style_paragraph_properties.h"

namespace cpdoccore { 
namespace odf_reader {



// office:text
//////////////////////////////////////////////////////////////////////////////////////////////////
const wchar_t * office_text::ns		= L"office";
const wchar_t * office_text::name	= L"text";


office_text::office_text()
{}

const cpdoccore::odf_reader::office_element_ptr_array& office_text::get_content()
{
	return content_;
}

void office_text::add_attributes( const xml::attributes_wc_ptr & Attributes )
{
    CP_APPLY_ATTR(L"text:global", text_global_, false);
}
 
bool is_text_content(const std::wstring & ns, const std::wstring & name)
{
    if (ns == L"text")
    {
        return (
            name == L"h" ||
            name == L"p" ||
            name == L"list" ||
            name == L"numbered-paragraph" ||
            name == L"section" ||
            name == L"page-sequence" ||
            name == L"soft-page-break" ||

            name == L"table-of-content" ||
            name == L"illustration-index" ||
            name == L"table-index" ||
            name == L"object-index" ||
            name == L"user-index" ||
            name == L"alphabetical-index" ||
            name == L"bibliography" ||
			name == L"alphabetical-index-auto-mark-file" ||

            name == L"change" ||
            name == L"change-start" ||
            name == L"change-end"

            );
    }
    else if (ns == L"table")
    {
        return name == L"table";
    }
    else if (ns == L"draw" || ns == L"dr3d")
    {
        return true; // all shapes // 
    }
	else if (ns == L"office" && name == L"forms") 
	{
		return true;
	}

    return false;
}

void office_text::add_child_element( xml::sax * Reader, const std::wstring & Ns, const std::wstring & Name)
{
	if CP_CHECK_NAME(L"text", L"tracked-changes") 
	{
		CP_CREATE_ELEMENT(tracked_changes_);
	}
	else if CP_CHECK_NAME(L"table", L"content-validations")
	{
		CP_CREATE_ELEMENT(table_content_validations_);
	}
	else if CP_CHECK_NAME(L"text", L"user-field-decls")
	{
		CP_CREATE_ELEMENT(user_fields_);
	}
	else if CP_CHECK_NAME(L"text", L"sequence-decls")
	{
		CP_CREATE_ELEMENT(sequences_);
	}
	else if CP_CHECK_NAME(L"text", L"variable-decls")
	{
		CP_CREATE_ELEMENT(variables_);
	}
	else if (is_text_content(Ns, Name))
    {
        CP_CREATE_ELEMENT(content_);

		if (!first_element_style_name && (content_.back()->get_type() == typeTextP || 
											content_.back()->get_type() == typeTextH))
		{//bus-modern_l.ott
			if (content_.back()->element_style_name)
				first_element_style_name = content_.back()->element_style_name;
			else
				first_element_style_name = L""; //default

		}
    }
    else
        CP_NOT_APPLICABLE_ELM();
}

void office_text::docx_convert(oox::docx_conversion_context & Context)
{
	if (sequences_)
		sequences_->docx_convert(Context);

	if (user_fields_)
		user_fields_->docx_convert(Context);
	
	if (variables_)
		variables_->docx_convert(Context);
	
	if (sequences_)
		sequences_->docx_convert(Context);
	
	if (tracked_changes_)
		tracked_changes_->docx_convert(Context);

 	//if (forms_)
		//forms_->docx_convert(Context);

	Context.start_office_text();

	_CP_OPT(std::wstring) masterPageName_first;

	if ((first_element_style_name) && (!first_element_style_name->empty()))
	{
		std::wstring style_name = *first_element_style_name;

		masterPageName_first = Context.root()->odf_context().styleContainer().master_page_name_by_name(style_name);
	   
		if (masterPageName_first)
		{				
			std::wstring masterPageNameLayout = Context.root()->odf_context().pageLayoutContainer().page_layout_name_by_style(*masterPageName_first);

			if (false == masterPageNameLayout.empty())
			{
				if (true == Context.set_master_page_name(*masterPageName_first))
				{
					Context.remove_page_properties();
					Context.add_page_properties(masterPageNameLayout);
				}
			}
		}  
	}
	for (size_t i = 0; i < content_.size(); i++)
    {
		if (content_[i]->element_style_name)
		{
			std::wstring style_name = *content_[i]->element_style_name;

			_CP_OPT(std::wstring) masterPageName = Context.root()->odf_context().styleContainer().master_page_name_by_name(*content_[i]->element_style_name);
		   
			if (!masterPageName)
			{
				style_instance* curr_style_instance = Context.root()->odf_context().styleContainer().style_by_name(style_name, odf_types::style_family::Paragraph, false);
				
				if (curr_style_instance && curr_style_instance->content()->get_style_paragraph_properties() &&
					curr_style_instance->content()->get_style_paragraph_properties()->content_.fo_break_before_)
				{
					if (curr_style_instance && false == curr_style_instance->parent_name().empty())
					{
						masterPageName = Context.root()->odf_context().styleContainer().master_page_name_by_name(curr_style_instance->parent_name());
					}
					if (!masterPageName && (style_name == L"Standard" || (curr_style_instance && curr_style_instance->parent_name() == L"Standard")))
					{
						masterPageName = L"Standard";
					}
				}
			}

			_CP_OPT(std::wstring) next_master_name = Context.get_next_master_page_name();
			if (next_master_name)
			{
				masterPageName = *next_master_name;
				Context.set_next_master_page_name(boost::none);
			}

			if (masterPageName)
			{
				std::wstring masterPageNameLayout = Context.root()->odf_context().pageLayoutContainer().page_layout_name_by_style(*masterPageName);

				if (false == masterPageNameLayout.empty())
				{
					if (true == Context.set_master_page_name(*masterPageName))
					{
						Context.remove_page_properties();
						Context.add_page_properties(masterPageNameLayout);
					}
				}
			}
		}
		if (content_[i]->next_element_style_name)
		{
			std::wstring text___ = *content_[i]->next_element_style_name;
			// проверяем не сменится ли свойства страницы.
			// если да — устанавливаем контексту флаг на то что необходимо в текущем параграфе
			// распечатать свойства раздела/секции
			//проверить ... не она ли текущая - может быть прописан дубляж - и тогда разрыв нарисуется ненужный
			const _CP_OPT(std::wstring) next_masterPageName	= Context.root()->odf_context().styleContainer().master_page_name_by_name(*content_[i]->next_element_style_name);

			if ((next_masterPageName)  && (Context.get_master_page_name() != *next_masterPageName))
			{
				if (false == Context.root()->odf_context().pageLayoutContainer().compare_page_properties(Context.get_master_page_name(), *next_masterPageName))
				{
					Context.next_dump_page_properties(true);
					//is_empty = false;
				}
			}

			style_instance* next_style_instance = Context.root()->odf_context().styleContainer().style_by_name(text___, odf_types::style_family::Paragraph, false);
			if (next_style_instance)
			{
				style_content* next_style_content = next_style_instance->content();
				if (next_style_content)
				{
					style_paragraph_properties* next_para_props = next_style_content->get_style_paragraph_properties();
					if (next_para_props)
					{
						if (next_para_props->content_.fo_break_before_ && next_para_props->content_.fo_break_before_->get_type() == odf_types::fo_break::Page)
						{
							std::wstring currentMasterPageName = Context.get_master_page_name();
							style_master_page* masterPage = Context.root()->odf_context().pageLayoutContainer().master_page_by_name(currentMasterPageName);

							if (masterPage && masterPage->attlist_.style_next_style_name_)
								Context.set_next_master_page_name(*masterPage->attlist_.style_next_style_name_);

							Context.next_dump_page_properties(true);
						}
					}
				}
			}
		} 
		content_[i]->docx_convert(Context);
    }
    Context.end_office_text();
}

void office_text::xlsx_convert(oox::xlsx_conversion_context & Context)
{
    //Context.start_office_text();
    for (size_t i = 0; i < content_.size(); i++)
    {
        content_[i]->xlsx_convert(Context);
    }
    //Context.end_office_text();
}

void office_text::pptx_convert(oox::pptx_conversion_context & Context)
{
    for (size_t i = 0; i < content_.size(); i++)
    {
        content_[i]->pptx_convert(Context);
    }
}

// office:change-info
//-------------------------------------------------------------------------------------------------------------------
const wchar_t * office_change_info::ns		= L"office";
const wchar_t * office_change_info::name	= L"change-info";

void office_change_info::add_attributes( const xml::attributes_wc_ptr & Attributes )
{
	int count = Attributes->size();
	
	CP_APPLY_ATTR(L"office:chg-author", office_chg_author_);
	CP_APPLY_ATTR(L"office:chg-date-time", office_chg_date_time_);
}

void office_change_info::add_child_element( xml::sax * Reader, const std::wstring & Ns, const std::wstring & Name)
{
	if CP_CHECK_NAME(L"dc", L"date")
	{
		CP_CREATE_ELEMENT(dc_date_);
	}
	else if CP_CHECK_NAME(L"dc", L"creator")
	{
		CP_CREATE_ELEMENT(dc_creator_);
	}
	else
		CP_NOT_APPLICABLE_ELM();
}

void office_change_info::docx_convert(oox::docx_conversion_context & Context)
{
	std::wstring date;
 	std::wstring author;
	
	if (dc_date_)
	{
		date = XmlUtils::EncodeXmlString(dynamic_cast<dc_date * >(dc_date_.get())->content_);
	}
	else if (office_chg_date_time_)
	{
		date = *office_chg_date_time_;
	}
	if (dc_creator_)
	{
		author = XmlUtils::EncodeXmlString(dynamic_cast<dc_creator * >(dc_creator_.get())->content_);
	}
	else if (office_chg_author_)
	{
		author = *office_chg_author_;
	}
	
	Context.get_text_tracked_context().set_user_info(author, date);
}

}
}
